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

FATFS = src/ds/src/fs/fat
DRIVERS = src/ds/src/drivers
UTILS = src/ds/src/utils

OBJS = src/main.o src/menu.o src/disc.o src/retrolog.o src/utility.o \
	src/dc/bmfont.o src/dc/descramble.o src/dc/drawing.o \
	src/dc/dreamfs.o src/dc/input.o src/dc/utility.o \
	$(DRIVERS)/rtc.o $(DRIVERS)/sd.o $(DRIVERS)/spi.o \
	$(FATFS)/../fs.o $(FATFS)/ff.o $(FATFS)/dc.o $(FATFS)/utils.o \
	$(FATFS)/option/ccsbcs.o $(FATFS)/option/syscall.o \
	$(UTILS)/../exec.o $(UTILS)/memcpy.o $(UTILS)/memset.o

KOS_CFLAGS += -D__DB_VERSION__="$(VERSION)"
ifneq ($(AUTOBOOT),0)
    KOS_CFLAGS += -DAUTOBOOT
endif

KOS_CFLAGS += -Isrc -Isrc/ds/include -Isrc/ds/include/fatfs

KOS_ROMDISK_DIR = romdisk

default: rm-elf $(TARGET).elf

include $(KOS_BASE)/Makefile.rules

$(TARGET).elf: $(OBJS) romdisk.o
	kos-cc -o $(TARGET).elf $(OBJS) romdisk.o -lpng -lz -lm -lkosext2fs

all: $(TARGET).elf $(TARGET).bin 1ST_READ.BIN $(TARGET).cdi bios-all

release: all rm-elf

clean: rm-elf
	-rm -f $(OBJS)
	-rm -f release/$(TARGET).bin release/1ST_READ.BIN release/$(TARGET).cdi release/$(TARGET)*.bios

rm-elf:
	-rm -f $(TARGET).elf romdisk.*

run: $(TARGET).elf
	$(KOS_LOADER) $(TARGET).elf

$(TARGET).bin: $(TARGET).elf
	$(KOS_OBJCOPY) -R .stack -O binary $(TARGET).elf release/$(TARGET).bin

1ST_READ.BIN: $(TARGET).bin
	scramble release/$(TARGET).bin release/1ST_READ.BIN

$(TARGET).cdi: $(TARGET).elf
	mkdcdisc --author $(TARGET) -e $(TARGET).elf --no-mr -n $(TARGET)-$(VERSION) -r 20240818 -o release/$(TARGET).cdi

bios: $(TARGET).bios
$(TARGET).bios: $(TARGET).bin
	cp -f res/boot_loader_retail.bios release/$(TARGET).bios
	dd if=release/$(TARGET).bin of=release/$(TARGET).bios bs=1024 seek=64 conv=notrunc

bios-nogdrom: $(TARGET)-nogdrom.bios
$(TARGET)-nogdrom.bios: $(TARGET).bin
	cp -f res/boot_loader_retail_nogdrom.bios release/$(TARGET)-nogdrom.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-nogdrom.bios bs=1024 seek=64 conv=notrunc

bios-devkit: $(TARGET)-devkit.bios
$(TARGET)-devkit.bios: $(TARGET).bin
	cp -f res/boot_loader_devkit.bios release/$(TARGET)-devkit.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit.bios bs=1024 seek=64 conv=notrunc

bios-devkit-nogdrom: $(TARGET)-devkit-nogdrom.bios
$(TARGET)-devkit-nogdrom.bios: $(TARGET).bin
	cp -f res/boot_loader_devkit_nogdrom.bios release/$(TARGET)-devkit-nogdrom.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-nogdrom.bios bs=1024 seek=64 conv=notrunc

bios-32mb: $(TARGET)-32mb.bios
$(TARGET)-32mb.bios: $(TARGET).bin
	cp -f res/boot_loader_retail_32mb.bios release/$(TARGET)-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-32mb.bios bs=1024 seek=64 conv=notrunc

bios-nogdrom-32mb: $(TARGET)-nogdrom-32mb.bios
$(TARGET)-nogdrom-32mb.bios: $(TARGET).bin
	cp -f res/boot_loader_retail_nogdrom_32mb.bios release/$(TARGET)-nogdrom-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-nogdrom-32mb.bios bs=1024 seek=64 conv=notrunc

bios-devkit-32mb: $(TARGET)-devkit-32mb.bios
$(TARGET)-devkit-32mb.bios: $(TARGET).bin
	cp -f res/boot_loader_devkit_32mb.bios release/$(TARGET)-devkit-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-32mb.bios bs=1024 seek=64 conv=notrunc

bios-devkit-nogdrom-32mb: $(TARGET)-devkit-nogdrom-32mb.bios
$(TARGET)-devkit-nogdrom-32mb.bios: $(TARGET).bin
	cp -f res/boot_loader_devkit_nogdrom_32mb.bios release/$(TARGET)-devkit-nogdrom-32mb.bios
	dd if=release/$(TARGET).bin of=release/$(TARGET)-devkit-nogdrom-32mb.bios bs=1024 seek=64 conv=notrunc

bios-all: bios bios-nogdrom bios-devkit bios-devkit-nogdrom bios-32mb bios-nogdrom-32mb bios-devkit-32mb bios-devkit-nogdrom-32mb
