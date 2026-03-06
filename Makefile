obj-m += i2c0-master.o

ARCH := arm64
CROSS_COMPILE := aarch64-linux-gnu-

KERN := /home/fog/linux
PWD := $(CURDIR)


.PHONY: all clean

all:
	$(MAKE) -C $(KERN) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) KBUILD_MODPOST_WARN=1 modules

clean:
	$(MAKE) -C $(KERN) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) KBUILD_MODPOST_WARN=1 clean
