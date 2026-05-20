# Team SwiftOS

Team member:
Youssef Boujebha,
Serjeoh Nakata,
Jahmeen Thomas


## Feedback History

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