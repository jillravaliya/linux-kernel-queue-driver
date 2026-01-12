# Queue Operations & Blocking Behavior

> This phase implements the complete circular queue functionality with blocking behavior using kernel wait queues.

The focus was on **building a fully functional blocking character device** that handles queue operations (SET_SIZE, PUSH, POP) and properly blocks processes when the queue is full or empty.

---

## What Was Added in This Phase

**Queue Operations:**
- `SET_SIZE_OF_QUEUE` implementation with `kmalloc()` memory allocation
- `PUSH_DATA` implementation with circular buffer logic and wrap-around
- `POP_DATA` implementation with data retrieval and tail management
- Error handling for invalid operations and memory allocation failures

**Blocking Behavior:**
- Wait queue initialization using `DECLARE_WAIT_QUEUE_HEAD()`
- `wait_event_interruptible()` for blocking PUSH when queue is full
- `wait_event_interruptible()` for blocking POP when queue is empty
- `wake_up_interruptible()` to wake waiting processes after operations

**Memory Management:**
- Dynamic queue allocation based on user-specified size
- Proper cleanup in exit function with `kfree()`
- Kernel-user space data transfer using `copy_from_user()` and `copy_to_user()`

**User Programs:**
- `configurator` - Sets queue size
- `filler` - Pushes data into queue
- `reader` - Pops data from queue (blocks if empty)

---

## Problems I Faced During This Phase

- Misspelled structure member `lenth` instead of `length`, causing cascading errors across multiple function calls
- Made critical typos in kernel APIs: `kmlloc` instead of `kmalloc`, `KER_INFO` instead of `KERN_INFO`
- Used incorrect function `class_unregister()` instead of `class_destroy()` for cleanup
- Forgot `static` keyword in module init/exit function declarations, breaking module registration
- Struggled to trace root cause when single typo generated 10+ cascading compiler errors
- Learned importance of careful proofreading - minor spelling mistakes in kernel code cause major build failures

---

## Testing Results

**Blocking Demonstration:**
```bash
Terminal 1: sudo ./reader
(Blocks - waiting for data...)

Terminal 2: sudo ./filler
(Pushes "xyz")

Terminal 1: xyz
(Wakes up and prints data!)
```

> This confirms proper blocking and wake-up behavior.

---

## Current State

**Fully Functional:**
- Queue can be dynamically sized
- Data can be pushed and popped
- Circular wrap-around works correctly
- Processes block when queue is full/empty
- Processes wake up when condition changes
- Memory is properly managed and freed

> **Kernel Driver:** COMPLETE

---

## Next Phase: User-Space Testing

*In the next phase, the focus is on testing the driver from user space.*

The goal is to confirm that the character device works as expected when
accessed by normal applications, including correct data flow and blocking
behavior. This step helps validate that the driver is usable in real
scenarios, not just internally correct.

> This phase ensures the driver behaves correctly under real usage, not just in isolation.

---

> This README will evolve as new functionality is added.


