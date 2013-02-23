CFLAGS ?= -std=c99 -Wall -shared -fPIC -DPIC
PKGFLAGS = `pkg-config --cflags --libs lv2`
LV2NAME = wavplayer

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
	mkdir $(BUNDLE)
	cp *.ttl $(BUNDLE);
	if [ $$STATIC_LV2 ]; then cp *.a $(BUNDLE); \
    else cp *.so $(BUNDLE); \
	fi

install : $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	cp -R $(BUNDLE) $(INSTALL_DIR)

sample.o : sample.wav
	$(LD) -s -r -o $@ -b binary $^

wavplayer.o : player.o sample.o
	$(LD) -r $^ -o $@

player.o : wavplayer.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.a : %.o
	ar rcs $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@

%.so : %.c
	$(CC) $(CFLAGS) $(PKGFLAGS)-c $^ -o $@

clean :
	rm -f *.o *.a *.so
	rm -rf $(BUNDLE)

