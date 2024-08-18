# dreamboot (WIP fork)

This is a fork of Cpajuste's custom BIOS for the Dreamcast. It requires KallistiOS to build.

#### Functionality
The BIOS will try to boot stuff in this order:
- If the text file "/sd/boot.cfg" exists, the bios will try to boot the defined binary path in this file.
- If the text file "/ide/boot.cfg" exists, the bios will try to boot the defined binary path in this file.
- If the file "/sd/RD/retrodream.bin" exists, the bios will boot that.
- If the file "/ide/RD/retrodream.bin" exists, the bios will boot that.
- If the file "/sd/DS/DS_CORE.BIN" exists, the bios will boot that.
- If the file "/ide/DS/DS_CORE.BIN" exists, the bios will boot that.
### Otherwise...
- If none of the previous cases succeed, the BIOS will enter the boot menu
- If "START" is pressed during boot up, the boot menu will be displayed
- If "A" + "B" is pressed during boot up, dc-load-serial will be launched
- If "X" + "Y" is pressed during boot up, dc-load-ip will be launched
