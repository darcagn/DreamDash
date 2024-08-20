## Configuration
config_file=Makefile.cfg
ifneq ("$(wildcard $(config_file))","")
    include $(config_file)
else
    ifneq ("$(wildcard Makefile.default.cfg)","")
        include Makefile.default.cfg
    else
        $(error No configuration file found at $(config_file) or Makefile.default.cfg)
    endif
endif

## Objects
FATFS = src/ds/src/fs/fat
DRIVERS = src/ds/src/drivers
UTILS = src/ds/src/utils

OBJS = src/main.o src/menu.o src/disc.o src/log.o src/utility.o \
	src/bmfont.o src/descramble.o src/drawing.o src/input.o \
	$(DRIVERS)/rtc.o $(DRIVERS)/sd.o $(DRIVERS)/spi.o \
	$(FATFS)/../fs.o $(FATFS)/ff.o $(FATFS)/dc.o $(FATFS)/utils.o \
	$(FATFS)/option/ccsbcs.o $(FATFS)/option/syscall.o \
	$(UTILS)/../exec.o $(UTILS)/memcpy.o $(UTILS)/memset.o

## Resources
RELEASE_DIR = release
RESOURCE_DIR = res
KOS_ROMDISK_DIR = romdisk

WALLPAPER_FILE = $(WALLPAPER_SHADE)-wall-$(WALLPAPER_RES).png
ROMDISK_FILES = ebdragon.fnt ebdragon.tex $(WALLPAPER_FILE)
GZ_ROMDISK_FILES = dcload-ip.bin dcload-serial.bin rungd.bin

## Flags
KOS_CFLAGS += -Isrc -Isrc/ds/include -Isrc/ds/include/fatfs
KOS_CFLAGS += -DWALLPAPER_FILE="$(WALLPAPER_FILE)" -DWALLPAPER_RES=$(WALLPAPER_RES)
KOS_CFLAGS += -DDASH_VERSION="$(VERSION)"
ifneq ($(AUTOBOOT),0)
    KOS_CFLAGS += -DAUTOBOOT
endif

## Rules
default: rm-elf $(TARGET).elf

include $(KOS_BASE)/Makefile.rules

$(ROMDISK_FILES):
	@mkdir -p $(KOS_ROMDISK_DIR)
	cp $(RESOURCE_DIR)/$@ $(KOS_ROMDISK_DIR)/$@

$(GZ_ROMDISK_FILES):
	@mkdir -p $(KOS_ROMDISK_DIR)
	gzip -c $(RESOURCE_DIR)/$@ > $(KOS_ROMDISK_DIR)/$@.gz

$(TARGET).elf: $(ROMDISK_FILES) $(GZ_ROMDISK_FILES) $(OBJS) romdisk.o
	kos-cc -o $(TARGET).elf $(OBJS) romdisk.o -lpng -lz -lm -lkosext2fs

all: $(TARGET).elf $(TARGET).bin 1ST_READ.BIN $(TARGET).cdi bios-all

release: all rm-elf

clean: rm-elf
	-rm -f $(OBJS)
	-rm -rf $(RELEASE_DIR)
	-rm -rf $(KOS_ROMDISK_DIR)

rm-elf:
	-rm -f $(TARGET).elf romdisk.*

run: $(TARGET).elf
	$(KOS_LOADER) $(TARGET).elf

release-dir:
	mkdir -p $(RELEASE_DIR)

$(TARGET).bin: release-dir $(TARGET).elf
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf release/$(TARGET).bin

1ST_READ.BIN: release-dir $(TARGET).bin
	scramble release/$(TARGET).bin release/1ST_READ.BIN

$(TARGET).cdi: release-dir $(TARGET).elf
	mkdcdisc --author $(TARGET) -e $(TARGET).elf --no-mr -n $(TARGET)-$(VERSION) -r 20240818 -o release/$(TARGET).cdi

bios: $(TARGET).bios
$(TARGET).bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_retail.bios release/$(TARGET).bios
	dd if=release/$(TARGET).bin of=release/$(TARGET).bios bs=1024 seek=64 conv=notrunc

bios-nogdrom: $(TARGET)-nogdrom.bios
$(TARGET)-nogdrom.bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_retail_nogdrom.bios release/$(TARGET)-nogdrom.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-nogdrom.bios bs=1024 seek=64 conv=notrunc

bios-devkit: $(TARGET)-devkit.bios
$(TARGET)-devkit.bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_devkit.bios release/$(TARGET)-devkit.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit.bios bs=1024 seek=64 conv=notrunc

bios-devkit-nogdrom: $(TARGET)-devkit-nogdrom.bios
$(TARGET)-devkit-nogdrom.bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_devkit_nogdrom.bios release/$(TARGET)-devkit-nogdrom.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-nogdrom.bios bs=1024 seek=64 conv=notrunc

bios-32mb: $(TARGET)-32mb.bios
$(TARGET)-32mb.bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_retail_32mb.bios release/$(TARGET)-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-32mb.bios bs=1024 seek=64 conv=notrunc

bios-nogdrom-32mb: $(TARGET)-nogdrom-32mb.bios
$(TARGET)-nogdrom-32mb.bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_retail_nogdrom_32mb.bios release/$(TARGET)-nogdrom-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-nogdrom-32mb.bios bs=1024 seek=64 conv=notrunc

bios-devkit-32mb: $(TARGET)-devkit-32mb.bios
$(TARGET)-devkit-32mb.bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_devkit_32mb.bios release/$(TARGET)-devkit-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-32mb.bios bs=1024 seek=64 conv=notrunc

bios-devkit-nogdrom-32mb: $(TARGET)-devkit-nogdrom-32mb.bios
$(TARGET)-devkit-nogdrom-32mb.bios: release-dir $(TARGET).bin
	cp -f res/boot_loader_devkit_nogdrom_32mb.bios release/$(TARGET)-devkit-nogdrom-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-nogdrom-32mb.bios bs=1024 seek=64 conv=notrunc

bios-all: bios bios-nogdrom bios-devkit bios-devkit-nogdrom bios-32mb bios-nogdrom-32mb bios-devkit-32mb bios-devkit-nogdrom-32mb
