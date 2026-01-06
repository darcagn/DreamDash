####################################################################################################
## NOTE: You shouldn't have to edit anything in this file. Check out Makefile.cfg for user config ##
####################################################################################################

###### Configuration ###############################################################################

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

# Project name
TARGET = dreamdash

# Project version
VERSION = 0.85

###### Objects #####################################################################################

OBJS = src/dreamdash.o
OBJS += src/bg.o src/drawing.o src/input.o src/storage.o src/texture.o src/utility.o
OBJS += src/filer.o src/mainmenu.o

ifeq ($(ROMFONT),0)
    OBJS += src/bmfont.o
endif

ifneq ($(DISC_SUPPORT),0)
    OBJS += src/disc.o
endif

ifneq ($(DISABLE_LOGGER),1)
    OBJS += src/log.o
endif

###### Libraries ###################################################################################

LIBS = -lz -lm

ifeq ("$(FAT_LIBRARY)","kosfat")
    LIBS += -lkosfat
endif

ifeq ("$(FAT_LIBRARY)","fatfs")
    LIBS += -lfatfs
endif

###### Resources ###################################################################################

RELEASE_DIR = release
RESOURCE_DIR = res
KOS_ROMDISK_DIR = romdisk

DCLOAD_IP_BIN = dcload-ip.bin
DCLOAD_SERIAL_BIN = dcload-serial.bin

WALLPAPER_BASENAME = $(WALLPAPER_NAME)-wall-$(WALLPAPER_SIZE)
WALLPAPER_FILE = $(WALLPAPER_BASENAME).png

GZ_ROMDISK_FILES = $(DCLOAD_IP_BIN) $(DCLOAD_SERIAL_BIN)

ifneq ($(DISC_SUPPORT),0)
    GZ_ROMDISK_FILES += rungd.bin
endif

###### Information Gathering #######################################################################

HAVE_MKDCDISC := $(shell command -v mkdcdisc 2> /dev/null)

DCLOAD_IP_BINSIZE := $(shell wc -c < $(RESOURCE_DIR)/$(DCLOAD_IP_BIN))
DCLOAD_SERIAL_BINSIZE := $(shell wc -c < $(RESOURCE_DIR)/$(DCLOAD_SERIAL_BIN))

###### Flags #######################################################################################

KOS_CFLAGS += -DDCLOAD_IP_BINSIZE=$(DCLOAD_IP_BINSIZE) -DDCLOAD_SERIAL_BINSIZE=$(DCLOAD_SERIAL_BINSIZE)
KOS_CFLAGS += -DDASH_VERSION="$(VERSION)"

ifneq ($(AUTOBOOT),0)
    KOS_CFLAGS += -DAUTOBOOT
endif

ifneq ($(DISC_SUPPORT),0)
    KOS_CFLAGS += -DDISC_SUPPORT
endif

ifneq ($(ROMFONT),0)
    KOS_CFLAGS += -DROMFONT
endif

ifneq ($(DISABLE_LOGGER),0)
    KOS_CFLAGS += -DDISABLE_LOGGER
endif

ifeq ("$(FAT_LIBRARY)","kosfat")
    KOS_CFLAGS += -DFAT_LIBRARY_KOSFAT
endif

ifeq ("$(FAT_LIBRARY)","fatfs")
    KOS_CFLAGS += -DFAT_LIBRARY_FATFS
endif

###### Functions ###################################################################################

define check_size
	@size=$$(wc -c < $(1)); \
	if [ $$size -gt $(2) ]; then \
		echo "Error: $(1) ($$size bytes) exceeds $(3) limit ($(2) bytes)!"; \
		echo "       In order to build a BIOS image, DreamDash must fit within that size."; \
		echo " Tips: - Adjust settings in Makefile.cfg to reduce code and resources."; \
		echo "       - Make sure your KallistiOS and all used kos-ports are built using the "; \
		echo "         -Os and -flto=auto flags in your $KOS_CFLAGS build flags."; \
		echo "       - Use GCC 13.2.0 (KOS stable compiler profile) as it generates smaller code."; \
		exit 1; \
	fi
endef

###### Rules #######################################################################################

default: rm-elf $(TARGET).elf

include $(KOS_BASE)/Makefile.rules

wallpaper:
	@mkdir -p $(KOS_ROMDISK_DIR)
	@$(KOS_BASE)/utils/pvrtex/pvrtex \
		-i $(RESOURCE_DIR)/$(WALLPAPER_FILE) \
		-o $(KOS_ROMDISK_DIR)/wallpaper.pvr \
		-f ARGB1555 \
		-c

font:
ifeq ($(ROMFONT),0)
	@mkdir -p $(KOS_ROMDISK_DIR)
	cp $(RESOURCE_DIR)/$(BMFONT_NAME).fnt $(KOS_ROMDISK_DIR)/font.fnt
	@$(KOS_BASE)/utils/pvrtex/pvrtex \
		-i $(RESOURCE_DIR)/$(BMFONT_NAME).png \
		-o $(KOS_ROMDISK_DIR)/font.pvr \
		-f ARGB1555 \
		-c
endif

$(ROMDISK_FILES):
	@mkdir -p $(KOS_ROMDISK_DIR)
	cp $(RESOURCE_DIR)/$@ $(KOS_ROMDISK_DIR)/$@

$(GZ_ROMDISK_FILES):
	@mkdir -p $(KOS_ROMDISK_DIR)
	gzip -c $(RESOURCE_DIR)/$@ > $(KOS_ROMDISK_DIR)/$@.gz

$(TARGET).elf: wallpaper font $(ROMDISK_FILES) $(GZ_ROMDISK_FILES) $(OBJS) romdisk.o
	kos-cc -o $(TARGET).elf $(OBJS) romdisk.o $(LIBS)

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
	$(KOS_BASE)/utils/scramble/scramble release/$(TARGET).bin release/1ST_READ.BIN

$(TARGET).cdi: release-dir $(TARGET).elf
ifneq ($(HAVE_MKDCDISC),)
	mkdcdisc --author $(TARGET) -e $(TARGET).elf --no-mr -n $(TARGET)-$(VERSION) -r 20250125 -o release/$(TARGET).cdi
else
	$(info mkdcdisc utility not found in PATH. Skipping CDI generation.)
endif

bios: $(TARGET).bios
$(TARGET).bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_retail.bios release/$(TARGET).bios
	dd if=release/$(TARGET).bin of=release/$(TARGET).bios bs=1024 seek=64 conv=notrunc

bios-nogdrom: $(TARGET)-nogdrom.bios
$(TARGET)-nogdrom.bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_retail_nogdrom.bios release/$(TARGET)-nogdrom.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-nogdrom.bios bs=1024 seek=64 conv=notrunc

bios-devkit: $(TARGET)-devkit.bios
$(TARGET)-devkit.bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_devkit.bios release/$(TARGET)-devkit.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit.bios bs=1024 seek=64 conv=notrunc

bios-devkit-nogdrom: $(TARGET)-devkit-nogdrom.bios
$(TARGET)-devkit-nogdrom.bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_devkit_nogdrom.bios release/$(TARGET)-devkit-nogdrom.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-nogdrom.bios bs=1024 seek=64 conv=notrunc

bios-32mb: $(TARGET)-32mb.bios
$(TARGET)-32mb.bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_retail_32mb.bios release/$(TARGET)-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-32mb.bios bs=1024 seek=64 conv=notrunc

bios-nogdrom-32mb: $(TARGET)-nogdrom-32mb.bios
$(TARGET)-nogdrom-32mb.bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_retail_nogdrom_32mb.bios release/$(TARGET)-nogdrom-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-nogdrom-32mb.bios bs=1024 seek=64 conv=notrunc

bios-devkit-32mb: $(TARGET)-devkit-32mb.bios
$(TARGET)-devkit-32mb.bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_devkit_32mb.bios release/$(TARGET)-devkit-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-32mb.bios bs=1024 seek=64 conv=notrunc

bios-devkit-nogdrom-32mb: $(TARGET)-devkit-nogdrom-32mb.bios
$(TARGET)-devkit-nogdrom-32mb.bios: release-dir $(TARGET).bin
	$(call check_size,release/$(TARGET).bin,517120,505KB)
	cp -f res/boot_loader_devkit_nogdrom_32mb.bios release/$(TARGET)-devkit-nogdrom-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-nogdrom-32mb.bios bs=1024 seek=64 conv=notrunc

bios-all: bios bios-nogdrom bios-devkit bios-devkit-nogdrom bios-32mb bios-nogdrom-32mb bios-devkit-32mb bios-devkit-nogdrom-32mb

.PHONY: wallpaper font
