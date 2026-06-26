
MODULE_NAME = driver

obj-m += $(MODULE_NAME).o

export CROSS_COMPILE=riscv64-linux-gnu-
export ARCH=riscv

.PHONY: build clean run



build:
	$(MAKE) $(MODULE_NAME).ko

$(MODULE_NAME).ko: $(MODULE_NAME).c
	$(MAKE) -C ~/linux modules M=$(shell pwd)
	cp $(MODULE_NAME).ko ~/rootfs
	cp $(MODULE_NAME).mod ~/rootfs
	cd ~/rootfs && find . | cpio -co > ../rootfs.cpio && cd ..

clean:
	$(MAKE) -C ~/linux clean M=$(shell pwd)


run: $(MODULE_NAME).ko
	cd ~/linux && qemu-system-riscv64 -machine virt -nographic -no-reboot -kernel arch/riscv/boot/Image -initrd ~/rootfs.cpio -append 'panic=-1' # init=~/rootfs/sh

