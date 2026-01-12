IOCTL Infrastructure & Queue Skeleton (Work in Progress)

> This phase introduces the IOCTL control path and the core data structures
required for a dynamic circular queue.

The focus here was on **setting up a stable kernel–user interface**, not on
implementing queue logic yet.

---

## What Was Added in This Phase

- IOCTL command definitions using `_IOW` and `_IOR`
  - `SET_SIZE_OF_QUEUE`
  - `PUSH_DATA`
  - `POP_DATA`
- Shared data structure for IOCTL communication (`struct queue_data`)
- IOCTL handler function (`unlocked_ioctl`)
- Command routing using `switch-case`
- Circular queue metadata structure (no memory allocation yet)

At this stage, IOCTL calls reach the kernel successfully but do not yet
perform any real operations.

---

## Problems I Faced During This Phase

- Broke the build by misspelling kernel APIs (`class_destory` instead of `class_destroy`)
- Used an incorrect macro name (`THIS_MODULES` instead of `THIS_MODULE`)
- Misread compiler output pointing to kernel header files, not my own code
- Had warnings treated as errors, which forced fixing even unused variables
- Wasted time due to simple command mistakes (`insomd` instead of `insmod`)

---

## Current Limitations

- No queue memory allocation (`kmalloc`) yet
- No actual push or pop logic implemented
- No blocking or synchronization mechanisms
- IOCTL handlers currently log requests only

These are intentional and will be implemented in the next phase.

---

## Validation Done

- Module loads and unloads cleanly
- IOCTL calls are received and logged via `printk`
- Device remains stable under invalid IOCTL commands

---

## Next Step

Implement dynamic queue allocation and core queue operations
(`SET_SIZE_OF_QUEUE`, `PUSH_DATA`, `POP_DATA`), followed by blocking behavior.


> This README will evolve as new functionality is added.
