# XLD-OS

**GitHub:** [https://github.com/azuracode464/XLD](https://github.com/azuracode464/XLD)

## Overview

XLD-OS is a hobby operating system designed for learning, experimentation, and kernel development.  
It includes a custom kernel, basic drivers, filesystem support, and UEFI/BIOS boot support.  

> ⚠️ **Not for production use.** Licensed under GPLv2.

---
> `edk2-ovmf/` is required for UEFI emulation with QEMU

---

## Prerequisites

- Linux host system (tested on Ubuntu/Debian)
- QEMU (qemu-system-x86_64)
- NASM (for assembly compilation)
- GCC cross-compiler targeting `x86_64-unknown-none-elf` OR CLANG and LLD
- MTools (for FAT32 disk image manipulation)
- xorriso (for ISO)
- Limine bootloader (included in the Build)

---

## How to Build and Run

1. **Create FAT32 disk image with `/root/hello.txt`:**

```bash
bash scripts/disk.sh

2. Compile the kernel and run XLD-OS:
make (for GCC)
or
make TOOLCHAIN=llvm (for clang)

bash run.sh

---

Features

GDT: Global Descriptor Table setup

Boot support: UEFI and BIOS

IDT & IRQ: Interrupt Descriptor Table and IRQ handling

PS/2: Keyboard input support

PMM: Physical Memory Manager

Heap management: kmalloc / kfree

ATA PIO driver: Disk read access (primary/master detected)

FAT32 driver: Filesystem detection, directory listing, reading files

VFS: Basic Virtual File System integration

GPLv2: Open source license




---

Current Status
   x = ready T = Testing
- [x] Kernel boots in BIOS and UEFI mode
- [x] Keyboard input works
- [x] Can read disk sectors via ATA PIO
- [x] FAT32 detection and directory listing
- [x] Read /root/hello.txt from disk
- [T] Writing to FAT32
- [ ] Multi-tasking
- [ ] User mode

---

Notes

XLD-OS is experimental and for learning purposes.

It demonstrates basic kernel features, drivers, and bootloader integration.

Contributions, bug reports, and suggestions are welcome via the GitHub repository.

