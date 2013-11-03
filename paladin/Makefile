CC := gcc

UNBOOTIMG_SRC := submodules/unbootimg
CORE_SRC := submodules/android_system_core
INC := -I$(CORE_SRC)/include -I$(CORE_SRC)/mkbootimg

NDK := /opt/android-ndk-r9b
CROSS := $(firstword $(wildcard $(NDK)/toolchains/mipsel-linux-android-4.6/prebuilt/*))/bin/mipsel-linux-android-
ADB := adb

.PHONY: all
all: boot.img

.PHONY: install
install: boot.img
	$(ADB) push $< /tmp/
	$(ADB) shell "dd if=/tmp/boot.img bs=1048576 seek=3 count=8 of=/dev/block/mmcblk0 ; sync"

unbootimg: $(UNBOOTIMG_SRC)/unbootimg.c $(CORE_SRC)/libmincrypt/sha.c
	$(CC) -o $@ $^ $(INC)

mkbootimg: $(CORE_SRC)/mkbootimg/mkbootimg.c $(CORE_SRC)/libmincrypt/sha.c
	$(CC) -o $@ $^ $(INC)

orig.img-ramdisk: orig.img unbootimg
	./unbootimg $<

.kernel-configured:
	$(MAKE) -C .. CROSS_COMPILE=$(CROSS) npm702_defconfig
	touch $@

.PHONY: boot.img-kernel
boot.img-kernel: .kernel-configured
	$(MAKE) -C .. CROSS_COMPILE=$(CROSS) zImage
	cp -f ../arch/mips/boot/compressed/zImage $@

boot.img: orig.img-ramdisk boot.img-kernel mkbootimg
	./mkbootimg --output boot.img \
		--kernel boot.img-kernel \
		--ramdisk orig.img-ramdisk.cpio.gz \
		--cmdline '' --board '' --base 10000000 --pagesize 2048
