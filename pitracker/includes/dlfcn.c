#include <stdint.h>
#include <elf/elf32.h>
#include <string.h>
#include <malloc.h>
#include <dlfcn.h>
#include <stdio.h>
#include <pi/fatfs.h>
#include <uthash.h>
#include <config.h>


typedef struct {
    char *filename;
    char *source_addr;
    char *strtable;
    char *shstrtab;
    Elf32_Shdr *s_hdrs;
} dl_handle;


// Debugging stuffs:

static void dump_sectioninfo(dl_handle* handle, Elf32_Shdr *s_hdr) {
    printf("SECTION \r\n");
    printf("name: %s\r\n", handle->shstrtab + s_hdr->sh_name);
    printf("sh_type: %d\r\n", (unsigned int)s_hdr->sh_type);
    printf("sh_addr: %x\r\n", (unsigned int)s_hdr->sh_addr);
    printf("sh_offs: %x\r\n", (unsigned int)s_hdr->sh_offset);
    printf("sh_size: %d\r\n", (unsigned int)s_hdr->sh_size);
    printf("sh_entsize: %d\r\n", (unsigned int)s_hdr->sh_entsize);
    printf("\r\n");
}

static void dump_symbol(dl_handle *handle, Elf32_Sym *symbol) {
    printf("st_name: %s\r\n", handle->strtable + symbol->st_name);
    printf("st_value: %x\r\n", (unsigned int)symbol->st_value);
    printf("st_size: %x\r\n", (unsigned int)symbol->st_size);
    printf("- st_type: %d\r\n", ELF32_ST_TYPE(symbol->st_info));
    printf("- Section index: %d\r\n", symbol->st_shndx);
    printf("\r\n");
}

static Elf32_Sym *get_sym(dl_handle *handle, Elf32_Shdr *s_hdr, int sym_index) {
    Elf32_Sym *symbol = (Elf32_Sym *)(handle->source_addr + s_hdr->sh_offset);
    symbol++;
    int cur_size = 0;
    cur_size += s_hdr->sh_entsize;
    int symcounter = 1;
    do {
        if (symcounter == sym_index) {
            return symbol;
        }
        cur_size += s_hdr->sh_entsize;
        symbol++;
        symcounter++;
    } while(cur_size < s_hdr->sh_size);
    return NULL;
}


typedef struct symbol_relocation {
    char *name;
    void *value;
    UT_hash_handle hh;
} symbol_relocation;

static symbol_relocation *symbol_relocation_table = NULL;


static void missing_symbol_code() {
    // The code that gets copied to the start of each
    // MissingFunctionContainer struct
    __asm__("push {lr}");
    __asm__("ldr r2, [pc, #52]");  // &printf
    __asm__("ldr r0, [pc, #52]");  // format_str
    __asm__("ldr r1, [pc, #52]");  // symbol_name
    __asm__("blx r2");             // call printf
    __asm__("ldr r0,=0xffffffff"); // Short pause to give the
    __asm__("l: sub r0, #1");      // console some time to print
    __asm__("bne l");              // the message in case of crash
    __asm__("pop {lr}");
}

typedef struct {
    char code[64];       // function code gets copied here
    void *printf;
    char *format_str;
    char *symbol_name;
} MissingSymbolFuncContainer;

static char unlinked_symbol_msg[] = "Error: Trying to call unlinked symbol: %s\r\n\0";

static void *make_missing_symbol_func(char *symbol_name) {
    // Evil code to make a kind of curried function that has symbol_name
    // baked into it. It also has pointers to printf and the format string
    // as that's easier than relocating the code.
    MissingSymbolFuncContainer *container = malloc(sizeof(MissingSymbolFuncContainer));
    memcpy(container, &missing_symbol_code, sizeof(container->code));
    container->printf = &printf;
    container->format_str = unlinked_symbol_msg;
    container->symbol_name = symbol_name;
    return container; // Return pointer to function code at beginning of container
}

void *dlopen(const char *filename, int flag) {
    FIL fp;
    FRESULT rc = f_open(&fp, filename, FA_READ);
    if (rc) printf("Failed to open %s: %u\r\n", filename, rc);
    UINT bytes_read;
    char *so_bytes= malloc(fp.fsize);
    rc = f_read(&fp, so_bytes, fp.fsize, &bytes_read);
    if (rc || bytes_read < fp.fsize) if (rc) printf("Failed to read %s: %u\r\n", filename, rc);
    rc = f_close(&fp);
    if (rc) printf("Failed to close %s: %u\r\n", filename, rc);

    dl_handle *handle = malloc(sizeof(dl_handle));
    handle->source_addr = so_bytes;
    strcpy(handle->filename, filename);
    Elf32_Ehdr *e_hdr = (Elf32_Ehdr*)handle->source_addr;
    if (e_hdr->e_type != 3) {
        printf("Not a shared object file!");
    }
    if (strncmp(handle->source_addr, "\x7f""ELF", 4)) {
        printf("Not elf!\r\n");
    }
    if (e_hdr->e_machine != 40) {
        printf("Wrong machine\r\n");
    }
    Elf32_Shdr *s_hdrs = (Elf32_Shdr*)(handle->source_addr + e_hdr->e_shoff);
    Elf32_Shdr *stringhdr = s_hdrs + e_hdr->e_shstrndx;
    Elf32_Shdr *symsection_hdr;

    handle->shstrtab = handle->source_addr + stringhdr->sh_offset;
    Elf32_Sym *symbol;
    Elf32_Rel *rel;
    unsigned int cur_size;
    for (int i=0;i<e_hdr->e_shnum;i++) {
        Elf32_Shdr *s_hdr = s_hdrs + i;
        switch(s_hdr->sh_type) {
        case SHT_DYNSYM:
            symsection_hdr = &s_hdrs[s_hdr->sh_link];

            // Set up string table
            handle->strtable = (char *)(handle->source_addr + symsection_hdr->sh_offset);

            // Stick symbol names / addresses in map
            int sym_size = s_hdr->sh_entsize;
            symbol = (Elf32_Sym *)(handle->source_addr + s_hdr->sh_offset);
            symbol++;
            cur_size = sym_size;
            struct symbol_relocation *entry;
            do {
                unsigned char st_type = ELF32_ST_TYPE(symbol->st_info);
                if (st_type == STT_FUNC) {// || st_type == STT_OBJECT) {
                    HASH_FIND_STR(symbol_relocation_table, handle->strtable + symbol->st_name, entry);
                    if (entry==NULL) {
                        entry = (struct symbol_relocation*)malloc(sizeof(struct symbol_relocation));
                        entry->name = handle->strtable + symbol->st_name;
                        entry->value = so_bytes + symbol->st_value;
                        HASH_ADD_KEYPTR(hh, symbol_relocation_table, entry->name, strlen(entry->name), entry);
                    } else {
                        printf("Warning: tried to add symbol %s more than once.\r\n", handle->strtable + symbol->st_name);
                    }
//            if (st_type == STT_OBJECT) printf("STT_OBJECT: %s\r\n", entry->name);
                }
                cur_size += sym_size;
                symbol++;
            } while(cur_size < s_hdr->sh_size);

            break;
        case SHT_REL:
            cur_size = 0;
            symsection_hdr = &s_hdrs[s_hdr->sh_link];
            rel = (Elf32_Rel *)(handle->source_addr + s_hdr->sh_offset);
            do {
                if ((symbol = get_sym(handle, symsection_hdr, ELF32_R_SYM(rel->r_info)))) {
                    unsigned char r_type = ELF32_R_TYPE(rel->r_info);
                    if (r_type == R_ARM_JUMP_SLOT || r_type == R_ARM_ABS32) {
                        // Fix up Global Offset Table entries
                        struct symbol_relocation *reloc;
                        char *symbol_name = handle->strtable + symbol->st_name;
                        HASH_FIND_STR(symbol_relocation_table, symbol_name, reloc);
                        void **got_ptr = (void**)(so_bytes + rel->r_offset);
                        if (reloc) {
//                            printf("Found %s: %x\r\n", reloc->name, reloc->value);
                            *got_ptr = reloc->value;
                        } else {
//#ifdef DEBUG
//                            printf("Failed to find symbol %s\r\n", symbol_name);
//                            This seems to break stuff, maybe because of memory allocation?
//                            *got_ptr = make_missing_symbol_func(symbol_name);
//#endif
                        }
                    } else {
                        printf("unhandled relocation type: %d\r\n", r_type);
                    }
                }
                cur_size += s_hdr->sh_entsize;
                rel++;
            } while(cur_size < s_hdr->sh_size);
            break;
        default:
            break;
        }
    }
    return handle;
}


void *dlsym(void *h, const char *symbol_name) {
    printf("dlsym: %s\r\n", symbol_name);
    struct symbol_relocation *s;
    HASH_FIND_STR(symbol_relocation_table, symbol_name, s);
    if (s) {
//        printf("Found %s in hashmap: %x\r\n", s->name, s->value);
        return s->value;
    }
    printf("Failed to find symbol in hashmap: %s\r\n", symbol_name);
    return NULL;
}

