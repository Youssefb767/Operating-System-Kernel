# Team SwiftOS

Team member:
Youssef Boujebha,
Serjeoh Nakata,
Jahmeen Thomas

A simulated operating-system kernel implemented in C as a three-person team project at California State University, Sacramento.

The project focused on core operating-system concepts including process management, CPU scheduling, synchronization, system calls, interrupt handling, terminal I/O, and virtual memory.

FEATURES:

Process creation and lifecycle management

Process control blocks and context switching

Round-robin and lottery scheduling

Per-process lottery ticket allocation

Semaphores, mutexes, wait queues, and spinlocks

Process blocking and unblocking

System calls using software interrupts and trapframes

Keyboard interrupt handling

TTY input and output

Ring-buffer-based input processing

Shell command parsing

VGA display updates

Page-fault and page-table analysis

CPU utilization testing with active and idle processes

Technical Concepts

C systems programming

Operating-system kernel architecture

Process scheduling and preemption

Interrupts and trap handling

User-mode and kernel-mode execution

Synchronization and concurrency

Critical sections and race-condition prevention

Trapframes and register-based argument passing

Page tables and virtual-memory translation

Queues, circular buffers, and ring buffers

Terminal and display I/O



My Contributions

As part of the three-person development team, I contributed to the implementation and analysis of several kernel subsystems:

Built synchronization primitives including semaphores, mutexes, wait queues, and spinlock-protected critical sections.

Integrated process blocking and unblocking with scheduler operations for process sleeping, semaphore waits, and mutex operations.

Implemented lottery scheduling using per-process ticket counts and pseudo-random proportional CPU selection.

Developed system-call interfaces using software interrupts, register-based argument passing, trapframes, and kernel dispatch handlers.

Implemented TTY input flow using keyboard interrupts, ring buffers, shell command parsing, and VGA display updates.

Analyzed CPU utilization using active test processes and halt-based idle processes.

Studied page-fault handling by tracing faulting virtual addresses through page tables and page directories.

Development Environment

Language: C

Version control: Git and GitHub

Development and execution environment: Spede-VM

Build tools: Make and GCC

Building and Running:
(in the spede-vm terminal)
cd ~/2026-spring-group-swiftos
make
spede-target -q
spede-target -s ~/spede.sock
spede-run -s ~/spede.sock -d

Project Context

This project was completed as part of operating-systems coursework at California State University, Sacramento. It provided practical experience implementing low-level kernel functionality and analyzing the interaction between processes, scheduling, synchronization, interrupts, and I/O.

Disclaimer

This repository represents an academic project developed for educational purposes.
