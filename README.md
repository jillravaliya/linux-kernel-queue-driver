# Linux Kernel Circular Queue Driver

![PHASE](https://img.shields.io/badge/PHASE-01-black?style=for-the-badge&logo=github&logoColor=white)
![FOCUS](https://img.shields.io/badge/FOCUS-KERNEL%20MODULE-D32F2F?style=for-the-badge&logo=target&logoColor=white)
![TECH STACK](https://img.shields.io/badge/TECH_STACK-LINUX-FFD700?style=for-the-badge&logo=linux&logoColor=white)
![LANGUAGE](https://img.shields.io/badge/LANGUAGE-C-0078D4?style=for-the-badge&logo=c&logoColor=white)
![LAST COMMIT](https://img.shields.io/badge/LAST%20COMMIT-JAN%2013-AAFF00?style=for-the-badge&logo=git&logoColor=white)


> This repository implements a kernel-space circular queue accessible from user space through `/dev/jill`, demonstrating blocking I/O, producer-consumer synchronization, and IOCTL-based communication between Ring 3 and Ring 0.

---

## What This Is

> A Linux kernel driver that provides a **shared circular queue** in kernel space, allowing multiple user programs to exchange data with automatic blocking behavior.

**Key Points:**
- **Lives in Ring 0:** Queue buffer resides in kernel memory (privileged)
- **Accessed from Ring 3:** User programs connect via `/dev/jill` device file
- **Blocking I/O:** Readers sleep when empty, writers sleep when full (no CPU waste)
- **Synchronized:** Kernel handles coordination between multiple producers/consumers

**Device:** `/dev/jill`

---

## Why This Matters

### The Problem
User programs run in **Ring 3 (restricted)** and cannot:
- Share memory between processes (isolated for security)
- Access kernel memory or hardware directly
- Coordinate without wasting CPU cycles (busy-waiting)

### The Solution  
A character device driver in **Ring 0 (privileged)** that provides:
- Shared circular queue in kernel memory
- IOCTL interface for push/pop operations
- Automatic blocking I/O (sleep when empty/full)
- Kernel-managed synchronization (wait queues)

### Real-World Applications
- **Video streaming:** Camera → Buffer → Encoder
- **Device drivers:** UART/GPIO data queues
- **Web servers:** Request queue → Worker threads
- **Print spooler:** Applications → Queue → Printer

---

## Development Process

[![WATCH](https://img.shields.io/badge/WATCH-DEVELOPMENT_PROCESS-FF0000?style=for-the-badge&logo=googledrive&logoColor=white)](https://drive.google.com/drive/folders/1sid-HKLANxNpNLXsBz822nu9Vlgs_tst?usp=share_link)

**Recording includes:**
- Complete coding process from scratch
- Kernel module compilation and loading
- Character device registration and `/dev/jill` creation
- IOCTL command implementation (SET_SIZE, PUSH_DATA, POP_DATA)
- Circular queue logic with head/tail management
- Blocking I/O implementation with wait queues
- Live testing with multiple processes
- Debugging and verification using `dmesg`

---

## Implementation

### Kernel Space (`hello.c`)

**IOCTL Commands:**
```c
SET_SIZE_OF_QUEUE  // Configure queue size (creates buffer in kernel memory)
PUSH_DATA          // Write data to queue (blocks if full)
POP_DATA           // Read data from queue (blocks if empty)
```

**Circular Queue Structure:**
```c
struct circular_queue {
    char *buffer;    // Kernel memory buffer
    int size;        // Total capacity
    int head;        // Write position
    int tail;        // Read position  
    int count;       // Current bytes
};
```

**Key Implementation Details:**
- Memory allocated in kernel space via `kmalloc()`
- Wrapping logic: `head = (head + 1) % size`
- Safe data transfer: `copy_from_user()` / `copy_to_user()`
- Blocking via `wait_event_interruptible()`
- Wake-up via `wake_up_interruptible()`

---

### User Space Programs

**configurator.c** - Sets queue size
```c
int fd = open("/dev/jill", O_RDWR);
int size = 100;
ioctl(fd, SET_SIZE_OF_QUEUE, &size);
```

**filler.c** - Pushes data
```c
struct data d;
d.length = 3;
d.data = "xyz";
ioctl(fd, PUSH_DATA, &d);
```

**reader.c** - Pops data (blocks until available)
```c
struct data d;
d.length = 3;
d.data = malloc(3);
ioctl(fd, POP_DATA, &d);
printf("%s\n", d.data);  // Prints: xyz
```

---

## How Blocking Works

### The Journey: Ring 3 → Ring 0 → Sleep → Wake

**When Queue is Empty:**
```
Reader (Ring 3): ioctl(fd, POP_DATA, ...)
      ↓ SYSCALL (CPU switches Ring 3 → Ring 0)
Kernel: Routes to driver
      ↓
Driver: Checks queue->count >= length
      ↓ FALSE (no data!)
Driver: wait_event_interruptible(read_wait, ...)
      ↓
Process State: TASK_RUNNING → TASK_INTERRUPTIBLE
CPU: Removes process from scheduler (0% CPU usage!)
      ↓
      ... Process SLEEPS ...
      ↓
Writer: ioctl(fd, PUSH_DATA, ...) adds data
Driver: wake_up_interruptible(&read_wait)
      ↓
Process State: TASK_INTERRUPTIBLE → TASK_RUNNING
CPU: Schedules process again
      ↓
Reader: Wakes up, gets data, returns to Ring 3
```

**Benefit:** No busy-waiting! Process sleeps efficiently until data arrives.

---

## Building and Testing

### Compile
```bash
cd kernel/
make          # Produces hello.ko
```

### Load Driver
```bash
sudo insmod hello.ko
ls /dev/jill  # Verify device created
```

### Test Blocking Behavior

**Terminal 1:**
```bash
./configurator     # Configure queue (100 bytes)
./reader           # BLOCKS - waiting for data
```

**Terminal 2:**
```bash
./filler           # Push "xyz"
```

**Result:** Terminal 1 immediately wakes and prints "xyz"

### Cleanup
```bash
sudo rmmod hello
```

---

## What Changes in the System

### Before Driver Loads
```
Kernel Space: Standard kernel only
/dev:         No /dev/jill
Capability:   Processes cannot share blocking queue
```

### After Driver Loads
```
Kernel Space: + Your driver code (Ring 0)
              + 100-byte circular buffer in kernel memory
              + Wait queues for synchronization
/dev:         + /dev/jill (character device)
Capability:   + Processes can share data via blocking queue
              + Automatic sleep/wake on empty/full
              + Efficient producer-consumer pattern
```

**Physical Memory:**
```
Kernel Memory (Ring 0):
  Address: 0xFFFF9000AABB0000 (example)
  Size:    100 bytes
  Access:  Only kernel code can access
  Type:    Circular buffer (head/tail pointers wrap)
```

---

---

## Core Concepts

### Character Device
> A character device handles data as a **stream of bytes**, one at a time or in sequence. Unlike block devices (hard disks) that work with fixed-size chunks, character devices process data sequentially like water flowing through a pipe.

**In this project:**
- Device file: `/dev/jill`
- Type: Character device
- Interface: IOCTL commands

---

### IOCTL Interface
> **Input/Output Control** - A mechanism to send custom commands to device drivers beyond standard read/write operations.

**Why IOCTL?**
- `read()` and `write()` only transfer data
- IOCTL sends **configuration** and **control** commands
- Each command has a unique code (e.g., `SET_SIZE_OF_QUEUE`)

**Structure:**
```
ioctl(file_descriptor, COMMAND_CODE, argument)
      |                |               |
   Connection      What to do      Extra data
```

---

### Ring 0 vs Ring 3
> The CPU enforces two privilege levels with hardware protection. Only 2 transistor bits control this separation.

| Ring 3 (User Mode) | Ring 0 (Kernel Mode) |
|-------------------|---------------------|
| User programs run here | Kernel and drivers run here |
| Restricted access | Full privileges |
| Cannot access kernel memory | Can access all memory |
| Cannot control hardware | Can control hardware directly |
| Safer (crash only affects process) | Dangerous (crash affects entire system) |

**Transition:**
- User program executes: `SYSCALL` instruction
- CPU automatically switches: Ring 3 → Ring 0
- Kernel code executes in privileged mode
- Returns via: `SYSRET` instruction
- CPU switches back: Ring 0 → Ring 3

---

### Blocking I/O & Synchronization
> When a resource is unavailable, the process **sleeps** instead of wasting CPU cycles checking repeatedly.

**Without Blocking (Bad):**
```c
while (!data_available) {
    check();  // Wastes 100% CPU!
}
```

**With Blocking (This Project):**
```c
data = pop_data();  // Process sleeps automatically
                    // Wakes when data arrives
                    // 0% CPU usage while waiting
```

**Implementation:**
- **Wait Queues:** Kernel structures managing sleeping processes
- **Sleep:** Process state changes to `TASK_INTERRUPTIBLE`
- **Wake:** Kernel notifies waiting processes when condition met
- **Result:** Efficient CPU usage, no race conditions

---

## Skills Demonstrated

- Linux kernel module development
- Character device driver architecture  
- IOCTL command design
- Circular buffer algorithm (O(1) push/pop)
- Kernel synchronization primitives
- Blocking I/O implementation
- Memory management in kernel space
- Safe user ↔ kernel data transfer
- Understanding of Ring 0/Ring 3 privilege separation

---

## Error Handling

```c
-EFAULT       // Bad user-space address
-EINVAL       // Invalid argument or queue not configured  
-ENOMEM       // Kernel memory allocation failed
-ERESTARTSYS  // System call interrupted (signal received)
```

---

## Project Structure

```
.
├── kernel/
│   ├── hello.c           # Kernel driver implementation
│   ├── Makefile          # Build configuration
│   ├── configurator.c    # Set queue size
│   ├── filler.c          # Push data to queue
│   ├── reader.c          # Pop data from queue
│   └── README.md         # Detailed technical documentation
└── README.md             # This file (overview)
```

---

## Use Cases

- **Device Drivers:** UART buffers, GPIO data queues
- **IPC:** Inter-process communication with blocking
- **Producer-Consumer:** Real-time data processing pipelines  
- **Embedded Systems:** Hardware FIFO management
- **FPGA Interfaces:** Data transfer between hardware and software

---

---

## Requirements

**System Requirements:**
- Linux kernel headers: `linux-headers-$(uname -r)`
- GCC compiler (version 7.0 or higher)
- Make utility
- Root/sudo privileges for module loading

**Recommended:**
- Basic understanding of C programming
- Familiarity with Linux terminal
- Knowledge of system calls and kernel concepts

---

## License

This project is licensed under **GPL v2** (GNU General Public License v2.0)

> GPL is required for all Linux kernel modules to maintain kernel compatibility and open-source compliance.

---

## Author

**Jill Ravaliya**

- GitHub: [jillravaliya](https://github.com/jillravaliya)
- LinkedIn: [jill-ravaliya09](https://www.linkedin.com/in/jill-ravaliya09/)
- Email: jillravaliya@gmail.com
- Project: [linux-kernel-queue-driver](https://github.com/jillravaliya/linux-kernel-queue-driver)

**Focus:** Systems Programming | Kernel Development | Embedded Systems

---

> *For detailed technical documentation on kernel concepts, Ring 0/Ring 3 transitions, and driver architecture, see `/kernel/README.md`*
