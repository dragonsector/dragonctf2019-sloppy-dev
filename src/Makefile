KDIR := /lib/modules/$(shell uname -r)/build

.PHONY: default
default: all

.PHONY: all
all:
	$(MAKE) -C $(KDIR) M=$$PWD

.PHONY: install
install:
	$(MAKE) -C $(KDIR) M=$$PWD modules_install

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
