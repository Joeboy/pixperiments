obj-m += pimidi.o
KVERSION := $(shell uname -r)
VERSION = 0.1

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

deb:
	sudo dkms add -m pimidi -v $(VERSION)
	sudo dkms build -m pimidi -v $(VERSION)
	sudo dkms mkdeb --source-only -m pimidi -v $(VERSION)
	cp /var/lib/dkms/pimidi/$(VERSION)/deb/pimidi-dkms_$(VERSION)_all.deb ./

dkms_remove:
	sudo dkms remove -m pimidi -v $(VERSION) --all
