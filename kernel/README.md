# Kernel Space – Character Device (Work in Progress)

> This directory contains the kernel-space implementation of the project.

The code here has moved beyond a simple "hello" module and now implements
a **basic Linux character device**. The focus so far has been on getting the
device lifecycle correct before adding any queue logic or IOCTL handling.

---

## Current State

*What is implemented right now:*

- Character device registration using `alloc_chrdev_region`
- `cdev` initialization and registration
- Device class and device node creation (`/dev/jill`)
- Basic file operations:
  - `open`
  - `release`
- Proper cleanup on module unload

> At this stage, the device does not yet expose any functionality beyond
being opened and closed.

---

## What I Did (Step by Step)

- Started with a minimal kernel module to understand init/exit flow
- Converted the module into a character device
- Registered a dynamic major number
- Created a device class and device node
- Verified device behavior using:
  - `insmod` / `rmmod`
  - `/dev/jill`
  - `dmesg`

*During this phase I hit real issues such as:*
- name conflicts with existing kernel symbols
- incorrect cleanup order causing errors
- permission issues when testing the device

> Each issue was fixed incrementally before moving forward.

---

## Files

- `hello.c`  
  Kernel module implementing a basic character device with open/release handlers.

- `Makefile`  
  Kbuild-style Makefile for building against the running kernel.

- `.gitignore`  
  Excludes generated build artifacts.

---

## What’s Next

The next steps will extend this driver with:
- IOCTL support
- a dynamically sized circular queue
- blocking behavior using wait queues

> This README will evolve as new functionality is added.ø
