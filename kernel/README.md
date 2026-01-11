# Current State

> *This folder contains my kernel-space work for this project.*

Right now this is just a **minimal kernel module**.  
Nothing fancy. No driver yet. This stage was only about getting comfortable
working inside kernel space without breaking the system.

---

## What I Actually Did

- Wrote a basic kernel module (`hello.c`)
- Built it using a Kbuild-style Makefile
- Compiled it against the running kernel headers
- Loaded the module with `insmod`
- Removed it with `rmmod`
- Verified execution using `dmesg`

> *The module prints a message when it is loaded and another when it is removed.
That’s it — and that’s intentional.*

---

## Why This Exists

Before touching:
- character devices
- major/minor numbers
- IOCTLs
- queues

I wanted to make sure I can:
- build kernel code correctly
- load and unload modules safely
- read kernel logs properly
- recover from common mistakes without panicking

I hit real issues here (compile warnings treated as errors, module already
loaded errors, permission issues with `dmesg`) and fixed them step by step.

> *This folder exists because of that learning.*

---

## Files

- `hello.c`  
  Minimal kernel module used only to understand the module lifecycle.

- `Makefile`  
  Kbuild Makefile used to build against the current kernel.

- `.gitignore`  
  Keeps build artifacts out of git.

  ---

## What’s Next

This module is the base.

Next steps will build on this:
- turn this into a character device
- register device numbers
- add a real kernel interface
- slowly move toward a queue-based driver

> *This README will change as the kernel code grows.*
