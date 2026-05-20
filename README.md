# Team SwiftOS

Team member:
Youssef Boujebha,
Serjeoh Nakata,
Jahmeen Thomas


## Feedback History

### [05/14/2026] Phase 5 Feedback
**Checklist Completion**
- Part 1: Instantiated the ”ping” and "pong" program (prog ping)
- Part 2: Mutexes implemented: `kmutexes_init`, `kmutex_init`, `kmutex_detroy`, `kmutex_lock`, `kmutex_unlock`, `ksyscall_mutex_init`, `ksyscall_mutex_unlock`, `ksyscall_mutex_lock`.
- Part 3: Semaphores implemented: `ksemphores_init`, `ksem_init`, `ksem_destory`, `ksem_wait`, `ksem_post`, `ksyscall_sem_init`, `ksyscall_sem_destory`, `ksyscall_sem_wait`, `ksyscall_sem_post`.
- Part 4: Lock (mutual exclusion) implemeted. `ksem_lock`, `ksem_init`, `ksem_unlock`, `kmutex_init`, `kmutex_lock`.
- Part 4: Lock is not implemented, `kmutex_unlock`

**Strengths**
- The system compiles and runs.
- The tag is set correctly.

**Weakness**
- When I typed in your shell, the Delete/Backspace key did not appear to update the TTY screen content correctly. For example, if I typed "hela", pressed Delete/Backspace, and then typed p, the screen still did not display the corrected command as "help". This is a relatively minor issue functionally, but it has a noticeable impact on the user experience and makes the shell feel less polished. 
- I noticed that the team did not implement mutual exclusion for the `kmutex_unlock`. As a result, Part 4 appears to be only partially completed. Without adding a spinlock or another synchronization mechanism to protect the mutex data, race conditions can occur when multiple threads (here are processes) access or modify the mutex concurrently.  


### [04/23/2026] Phase 4 Feedback

**Strengths**
- The system compiles successfully.
- `ksyscall_init()` registers `isr_entry_syscall` and `ksyscall_irq_handler`.
- `syscall.c` implements `_syscall0()`, `_syscall1()`, `_syscall2()`, and `_syscall3()`.
- User-facing system call APIs are present for `sys_get_time()`, `sys_get_name()`, `proc_sleep()`, `proc_exit()`, `proc_get_pid()`, `proc_get_name()`, `io_write()`, `io_read()`, and `io_flush()`.
- `kproc_init()` creates an idle process and four shell processes.
- `kproc_attach()` exists and is used to attach shell processes to TTYs.
- `scheduler_sleep()` and sleep-queue wakeup logic are implemented, and the timer callback wakes sleeping processes.
- The tag is set correctly.


**Areas for Improvement**
- **The system fails to run due to NULL pointer access***. 
- `tty_get()` returns `NULL` and `tty_input()` is not implemented, so the TTY input/output path is incomplete.

### [03/30/2026] Phase 3 Feedback

**Strengths**
- The system compiles successfully.
- `kproc_create()`, `kproc_destroy()`, and `kproc_init()` are implemented.
- `scheduler_run()`, `scheduler_add()`, `scheduler_remove()`, and `scheduler_init()` are implemented.
- Processes are added to the scheduler during creation and removed during destruction.
- The run queue logic for scheduling is implemented.
- The tag is set correctly.

**Areas for Improvement**
- Consider adding logging for process destruction and context switches.

### [03/16/2026] Phase 2 Feedback

**Strengths**
- The system compiles and runs.
- All of the required functions are implemented correctly.
- **ALT + (0–9)** correctly switches TTYs.
- All TTY content is saved properly.
- The system scrolls down correctly when the text reaches the bottom of the window.
- The tag is set correctly.
- **EXTRA:** Scrolling with **PAGE UP**, **PAGE DOWN**, **HOME**, and **END** functions properly in the TTY (**up, down, top, bottom**).

**Areas for Improvement**
- Please consider adding logging for TTY switches.


### [02/28/2026] Phase 1 Feedback

**Strengths:**
- The system compiles and runs without issues  
- **CTRL-K** correctly clears the screen
- **CTRL+ESC** exits the OS properly
- **Caps Lock** correctly capitalizes letters
- **Shift key** works correctly 
- When **Caps Lock** is enabled, pressing **Shift** correctly alternates capitalization
- Kernel log level increase/decrease functions work properly
- Nice work adding scrolling down
- Nice work adding the cursor
- Correct tag

**Areas for Improvement:**
- **CTRL-B** does not set a breakpoint in debug mode  
- **CTRL-P** does not trigger kernel panic 
- **Enter**, **Backspace**, **Tab**, **NumPad** keys do not function.
- It would be nice to add more logging (e.g. key logging)
