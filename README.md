# Xld Kernel

Xld is a small experimental x86_64 operating system kernel written in C and Assembly.
It is built for learning purposes and low-level exploration of how operating systems work.

## Features

- x86_64 long mode kernel
- Custom GDT and IDT
- Keyboard driver (IRQ-based)
- Basic interrupt handling
- Framebuffer output (if implemented)
- Designed to be simple and hackable

## Goals

The main goals of this project are:

- Learn how modern x86_64 kernels work
- Understand interrupts, faults, and hardware interaction
- Build a clean and readable kernel codebase
- Keep the project small and educational

This is **not** meant to be a production-ready OS.

## Build & Run

This kernel is typically built using a cross-compiler and tested with an emulator like QEMU.

Example

make

OR

make TOOLCHAIN=llvm

TO RUN

make run
