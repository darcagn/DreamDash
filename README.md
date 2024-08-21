# DreamDash BIOS
DreamDash is a replacement BIOS for Dreamcast power users and developers. It requires KallistiOS to build.

![dreamdash-preview](https://github.com/user-attachments/assets/b0aa299f-2c0e-4fa7-9a9c-868ccfb49de6)

## Functionality
- Launch binaries directly from SD card or IDE drive
- dcload-ip and dcload-serial binaries embedded directly into ROM
- Menu entries appear for DreamShell or RetroDream if found on SD or IDE device
- Configurable to auto-boot into a particular application
- Launch games or applications from GD-ROM drive
- Choose between light or dark wallpaper theme
- Choose between retail or devkit style intro
- Support for Dreamcast consoles with 32MB RAM
- Build as a BIOS, a standalone binary utility, or a CD image

## Issues/Limitations
- BIOS image will not work if stripped binary size is larger than roughly 500KB
  - Build your KallistiOS, kos-ports, and DreamDash with `-Os` and `-flto=auto` to help stay below this limit
  - Always have a backup BIOS image on your console for recovery if flashing to console
- Certain older homebrew is not compatible with consoles using a custom BIOS without patching
  - This applies equally for launching from CD, SD, or IDE
- Phantasy Star Online Ver 2's anti-cheat tamper protection causes bugs when launched via this BIOS
 
## Acknowledgements
- **KallistiOS** and **DreamShell** - kernel and drivers for the underlying operating system
- **Cpajuste** - Dreamboot, from which this project was originally forked
- **Troy D. Hanson & Arthur O'Dwyer** - ut* C structures libraries
