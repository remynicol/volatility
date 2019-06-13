obj-m += module.o
KDIR ?= /
KVER ?= $(shell uname -r)

-include version.mk

KBUILD_CFLAGS += "-fplugin=$(PWD)/gcc-plugins/debug-randstruct.so"

all: dwarf

dwarf: module.c gcc-plugins/debug-randstruct.so
	$(MAKE) -C $(KDIR)/lib/modules/$(KVER)/build CONFIG_DEBUG_INFO=y M="$(PWD)" modules
	dwarfdump -di module.ko > module.dwarf
	$(MAKE) -C $(KDIR)/lib/modules/$(KVER)/build M="$(PWD)" clean

gcc-plugins/debug-randstruct.so: gcc-plugins/debug-randstruct.cpp
	$(MAKE) -C gcc-plugins debug-randstruct.so

clean:
	$(MAKE) -C $(KDIR)/lib/modules/$(KVER)/build M="$(PWD)" clean
	$(MAKE) -C gcc-plugins clean
	rm -f module.dwarf
