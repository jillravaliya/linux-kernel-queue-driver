# Kernel Driver - Technical Documentation

> Internal architecture, blocking I/O mechanics, and kernel-level implementation details of the circular queue driver - how the code actually works, from IOCTL command routing and wait queue synchronization to circular buffer wraparound logic and safe memory transfers across the Ring 3/Ring 0 boundary.

---

## Architecture

**Single global queue design:**
- One `circular_queue` shared by all processes
- No per-file-descriptor state
- All operations block by default
- Wait queues handle synchronization

**Why this approach?**
- Simple producer-consumer pattern
- True inter-process communication
- Kernel manages all coordination

---

## Core Data Structures

### The Circular Queue
```c
struct circular_queue {
    char *buffer;    // Kernel memory
    int size;        // Total capacity
    int head;        // Write position (where to add)
    int tail;        // Read position (where to remove)
    int count;       // Current bytes stored
};
```

**Why the `count` field?**

Without it, when `head == tail`:
- Could mean empty (no data)
- Could mean full (wrapped around)
- Ambiguous!

With `count`:
- Empty: `count == 0`
- Full: `count == size`
- Clear and unambiguous

### User-Kernel Data Structure
```c
struct data {
    int length;      // How many bytes
    char *data;      // User-space pointer!
};
```

**Critical:** The `data` pointer lives in user space. Must use `copy_from_user()` to access it safely.

---

## Device Registration

**How `/dev/jill` gets created:**

```c
// 1. Get device number
alloc_chrdev_region(&dev, 0, 1, "jill");

// 2. Connect operations to device
cdev_init(&my_cdev, &fops);
cdev_add(&my_cdev, dev, 1);

// 3. Create device class
my_class = class_create("jill");

// 4. Create device file (appears in /dev automatically)
device_create(my_class, NULL, dev, NULL, "jill");
```

**Result:** `/dev/jill` appears, no manual `mknod` needed.

---

## IOCTL Commands

### Command Encoding
```c
#define IOCTL_SET_SIZE  _IOW('a', 'a', int32_t*)
#define IOCTL_PUSH_DATA _IOW('a', 'b', struct data*)
#define IOCTL_POP_DATA  _IOR('a', 'c', struct data*)
```

- `'a'` = Magic number (identifies this driver)
- `'a'`, `'b'`, `'c'` = Command sequence
- `_IOW` = Writing to driver, `_IOR` = Reading from driver

### SET_SIZE: Initialize Queue

**What it does:**
1. Check queue doesn't already exist
2. Copy size from user space
3. Allocate queue structure: `kmalloc(sizeof(struct circular_queue))`
4. Allocate buffer: `kmalloc(size)`
5. Initialize: `head = tail = count = 0`

**Why only once?**
```c
if(queue != NULL) return -EINVAL;
```
Prevents memory leaks from multiple allocations.

### PUSH_DATA: Write to Queue

**The process:**
1. Copy user's data structure to kernel
2. **Wait if queue is full:** `wait_event_interruptible(write_wait, ...)`
3. Allocate temporary kernel buffer
4. Copy user data to temporary buffer
5. Write to circular buffer byte-by-byte:
```c
for(i = 0; i < length; i++) {
    buffer[head] = kbuf[i];
    head = (head + 1) % size;  // Wrap around!
    count++;
}
```
6. Wake up waiting readers: `wake_up_interruptible(&read_wait)`

### POP_DATA: Read from Queue

**The process:**
1. Copy request from user space
2. **Wait if queue is empty:** `wait_event_interruptible(read_wait, ...)`
3. Allocate temporary kernel buffer
4. Read from circular buffer:
```c
for(i = 0; i < length; i++) {
    kbuf[i] = buffer[tail];
    tail = (tail + 1) % size;  // Wrap around!
    count--;
}
```
5. Copy data back to user space
6. Wake up waiting writers: `wake_up_interruptible(&write_wait)`

---

## Blocking I/O: How It Works

### Wait Queues

**Declaration:**
```c
DECLARE_WAIT_QUEUE_HEAD(read_wait);   // For waiting readers
DECLARE_WAIT_QUEUE_HEAD(write_wait);  // For waiting writers
```

These are kernel structures that manage sleeping processes.

### The Sleep-Wake Cycle

**When writer finds queue full:**
```
1. wait_event_interruptible(write_wait, space_available)
2. Process state → TASK_INTERRUPTIBLE (sleeping)
3. CPU removes process from scheduler
4. Process uses 0% CPU while waiting
   
   ... time passes ...
   
5. Reader removes data
6. Reader calls: wake_up_interruptible(&write_wait)
7. Kernel checks condition again
8. If true, process state → TASK_RUNNING
9. Process continues execution
```

**Why "interruptible"?**
- Process can be killed (Ctrl+C works)
- Returns `-ERESTARTSYS` if interrupted
- Prevents unkillable zombie processes

---

## Circular Buffer Logic

### Why Modulo?

```c
head = (head + 1) % size;
tail = (tail + 1) % size;
```

**The magic of `% size`:**
```
Size = 5
Positions: 0, 1, 2, 3, 4

head = 4
head = (4 + 1) % 5 = 0  ← Wraps to beginning!
```

This creates the "circular" behavior - when you reach the end, you start at the beginning.

### Visual Example

```
Buffer: [A][B][C][D][E]  size=5
         ↑           ↑
        tail        head

After writing F:
Buffer: [A][B][C][D][E]
         ↑
        head (wrapped to 0!)
              tail stays at 1

New buffer: [F][B][C][D][E]
             ↑     ↑
            head  tail
```

---

## Memory Safety

### The Golden Rule

**NEVER access user-space pointers directly:**
```c
// WRONG - will crash!
buffer[0] = user_data.data[0];

// CORRECT - safe access
copy_from_user(kbuf, user_data.data, length);
```

**Why `copy_from_user()`?**
- Validates pointer is in user space (not kernel space)
- Checks memory is readable
- Handles page faults safely
- Returns error if pointer is bad

**Reverse direction:**
```c
copy_to_user(user_data.data, kbuf, length);
```

### Temporary Buffers

**Pattern used everywhere:**
```c
kbuf = kmalloc(length, GFP_KERNEL);  // Allocate
copy_from_user(kbuf, user_ptr, length);  // Copy from user
// ... use kbuf ...
kfree(kbuf);  // Always free!
```

**Why temporary buffer?**
Can't write directly from user pointer to queue buffer. Need intermediate kernel-space storage.

---

## Error Handling

### Common Return Codes

| Code | When | Meaning |
|------|------|---------|
| `-EINVAL` | Queue not set up, bad length | Invalid argument |
| `-EFAULT` | User pointer is bad | Bad address |
| `-ENOMEM` | kmalloc failed | Out of memory |
| `-ERESTARTSYS` | Process interrupted (Ctrl+C) | System call interrupted |

### Cleanup on Failure

**Example:**
```c
queue = kmalloc(sizeof(struct circular_queue), GFP_KERNEL);
if(!queue) return -ENOMEM;

queue->buffer = kmalloc(size, GFP_KERNEL);
if(!queue->buffer) {
    kfree(queue);  // Clean up first allocation!
    queue = NULL;
    return -ENOMEM;
}
```

**Rule:** If something fails, free everything allocated so far.

---

## Module Cleanup

**Exit sequence (reverse order of init):**
```c
1. Free queue memory (buffer, then structure)
2. Destroy device file (/dev/jill removed)
3. Remove device class
4. Delete character device
5. Release device number
```

**Order matters!** Must free resources before removing infrastructure.

---

## Performance Notes

### O(1) Operations

Both push and pop are **constant time**:
- Direct array access: `buffer[head]`, `buffer[tail]`
- No data shifting required
- Fast regardless of queue size

### Current Implementation

Copies byte-by-byte in loop:
```c
for(i = 0; i < length; i++) {
    buffer[head] = kbuf[i];
    head = (head + 1) % size;
}
```

**Could optimize:** Use `memcpy()` when data doesn't wrap around.

---

## Common Issues

### Process Hangs Forever
**Problem:** Forgot wake-up call  
**Fix:** Always `wake_up_interruptible()` after push/pop

### Kernel Panic
**Problem:** Dereferenced user pointer directly  
**Fix:** Use `copy_from_user()` / `copy_to_user()`

### Memory Leak
**Problem:** Forgot to free temporary buffer  
**Fix:** Always `kfree()` what you `kmalloc()`

### `-EINVAL` Error
**Problem:** Trying to push/pop before SET_SIZE  
**Fix:** Call configurator first to set queue size

---

## Key Concepts Summary

**What makes this work:**

1. **Wait Queues** - Efficient blocking without CPU waste
2. **Modulo Arithmetic** - Creates circular wraparound
3. **Count Field** - Eliminates empty/full confusion
4. **Safe Copying** - `copy_from_user()` / `copy_to_user()`
5. **Two Wake Queues** - Separate for readers and writers

**Design principles:**

- Simple global state (one queue for all)
- Safety first (validated user access)
- Blocking by default (efficient waiting)
- Standard kernel patterns (cdev, wait queues)

---

## File Operations Structure

```c
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl,
};
```

**Connects system calls to your functions:**
- `open()` → `dev_open()`
- `close()` → `dev_release()`
- `ioctl()` → `dev_ioctl()`

---

*For project overview and usage instructions, see main `/README.md`*
