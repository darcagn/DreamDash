# DreamDash
DreamDash is a multi-use bootCD tool and replacement BIOS

![dreamdash-preview](https://github.com/user-attachments/assets/b0aa299f-2c0e-4fa7-9a9c-868ccfb49de6)

## Uses
- **Burn to CD** and use DreamDash to boot import discs and bypass VGA restrictions, load DreamShell, and launch homebrew from SD adapters, internal SD mods, and IDE, CompactFlash, and SATA mods
- **Install as a BIOS** with a BIOS mod to boot directly into DreamDash and launch games and utilities from storage devices with no discs needed! You can even ditch your GD-ROM drive completely.

## Functionality
- Boot into DreamShell, including with configurable autoboot
- Launch homebrew directly from SD card adapters, internal SD card mods, and IDE, CompactFlash, or SATA drives
- dcload-ip and dcload-serial support built directly in, with autoboot available for rapid development
- Choose between light or dark wallpaper theme
- Choose between retail or devkit style intro
- Support for Dreamcast consoles with 32MB RAM expansion

## Roadmap
- Support for use as a GDEMU frontend
- VMU and saves management
- System settings management
- ISP settings management
- Non-selfboot (Utopia style) disc booting
- Networking with Broadband Adapter, LAN Adapter, or WIZnet W5500 adapter
  - Automatic NTP time/date setting over network
- Plugin system for expandability
  - Integration with [Dream Web Console](https://github.com/darcagn/dream-web-console)

## How to Use
### bootCD
- Download DreamDash from the [releases](https://github.com/darcagn/DreamDash/releases) page.
- Unzip the release file and find the `dreamdash.cdi` file.
- Burn the [DiscJuggler CDI](https://dreamcast.wiki/DiscJuggler) file with a CD burner.

### BIOS
- Modify your Dreamcast with a [BIOS mod](https://dreamcast.wiki/BIOS_modification). Soldering required!
- Burn and boot the latest [DreamShell release](https://github.com/DC-SWAT/DreamShell/releases).
- Download DreamDash from the [releases](https://github.com/darcagn/DreamDash/releases) page.
- Using an SD adapter or other file storage method, open the **BIOS Flasher** and write the DreamDash BIOS file.

### Building from source
- [Set up KallistiOS on your computer](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development).
- When setting up the toolchain, adjust your `Makefile.cfg` configuration to enable newlib space optimizations.
- Compile KOS and the `zlib` kos-port with `-Os` and `-flto=auto` in your `KOS_CFLAGS` to keep code size small.
  - KOS master with commit ID `7b0b815` is known to compile with this code.
- Open a terminal and source your KOS environment, clone this repo, and change into this repo's directory.
- Alter your desired settings in `Makefile.cfg`.
- Run `make` to build `dreamdash.elf`.
- Run `make all` to build everything. Check the `release` directory for generated files:
  - `dreamdash.bin`: Plain raw binary
  - `1ST_READ.BIN`: Scrambled binary for booting from CD-R
  - `dreamdash.cdi`: DiscJuggler CDI image for burning to CD-R (requires `mkdcdisc` installed to `$PATH` to generate CDI)
  - `dreamdash.bios`: BIOS image with standard bootup intro. There are several variants:
    - BIOS files with `devkit` use the devkit bootup intro
    - BIOS files with `nogdrom` are used on consoles with no GD-ROM drive installed (**currently untested!**)
    - BIOS files with `32mb` are used on consoles with 32MB RAM modification
- Use DreamShell's BIOS Flasher application to write the `.bios` file to a Dreamcast's writeable BIOS flashROM.

## Discussion
A chat room is available on the [dreamcast.wiki Discord server](https://discord.gg/Bs6Fe4stzE). Join us!
 
## Acknowledgements
- **KallistiOS** and **DreamShell** - kernel and drivers for the underlying operating system
- **Cpajuste** - Dreamboot, from which this project was originally forked
