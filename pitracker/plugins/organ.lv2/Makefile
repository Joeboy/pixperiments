CFLAGS ?= -std=c99 -Wall -shared -fPIC -DPIC
PKGFLAGS = `pkg-config --cflags --libs lv2`
LV2NAME = organ

BUNDLE=$(LV2NAME).lv2

TTL=$(LV2NAME).ttl

ifndef INSTALL_DIR
INSTALL_DIR=`readlink -f ~/.lv2`
endif

ifdef STATIC_LV2
BINARY=$(LV2NAME).a
else
BINARY=$(LV2NAME).so
endif

all : $(BUNDLE)

$(BUNDLE): manifest.ttl $(TTL) $(BINARY)
	mkdir -p $(BUNDLE)
	cp *.ttl $(BINARY) $(BUNDLE);

install : $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	cp -R $(BUNDLE) $(INSTALL_DIR)

%.a : %.o
	ar rcs $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.so : %.c
	$(CC) $(CFLAGS) $(PKGFLAGS)-c $^ -o $@

clean :
	rm -f *.o *.a *.so
	rm -rf $(BUNDLE)

