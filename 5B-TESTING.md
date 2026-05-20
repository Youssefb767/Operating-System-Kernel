# Phase 5B Testing Notes

## Build Test

Command used:

```bash
make clean
make
```

Expected result:

- Build completes successfully.
- The project compiles with `-Wall` and `-Werror` enabled through `EXTRA_CFLAGS` in the Makefile.
- No warnings or errors are produced.

Actual result:

- Build completed successfully with no warnings or errors.

## Boot Test

Commands used:

```bash
spede-target
spede-run
```

Expected result:

- OS boots successfully.
- Kernel initializes normally.
- Interrupts initialize normally.
- Timer initializes normally.
- TTY, VGA, keyboard, scheduler, process management, syscalls, semaphores, mutexes, and serial ports initialize successfully.
- Remote terminal processes are created.

Actual result:

- OS booted successfully.
- Shell, ping, pong, and remote terminal processes were created.
- No kernel panic occurred during boot.

## Serial Terminal Startup Test

Serial clients used:

```bash
spede-term com2
spede-term com3
```

Expected result:

- Both serial terminals display the remote login prompt.
- COM2 and COM3 work at the same time.
- TTY 5 and TTY 6 may remain blank because the actual interaction happens through the serial terminals.

Actual result:

- `spede-term com2` displayed the remote login prompt.
- `spede-term com3` displayed the remote login prompt.
- TTY 5 and TTY 6 were blank, which is expected because the remote terminal uses serial I/O.

## Login Test

Credentials tested:

```text
username: spede
password: spede
```

Expected result:

- Correct credentials allow access to the remote terminal shell.
- Incorrect credentials return to the login prompt.
- Incorrect credentials do not crash the OS.

Actual result:

- Correct credentials successfully opened the remote terminal shell.
- Incorrect credentials returned to the login prompt.

## Remote Command Test

Commands tested:

```text
help
time
sleep
exit
```

Expected result:

- `help` displays available commands.
- `time` displays the current system time.
- `sleep` puts the remote terminal process to sleep and then returns.
- `exit` exits the remote terminal process or ends the remote command loop cleanly.
- Unknown commands are handled without crashing the OS.

Actual result:

- `help` displayed the command list.
- `time` displayed the current system time.
- `sleep` caused the process to sleep and then return.
- `exit` exited the remote terminal process or command loop.
- Unknown commands were handled without crashing.

## Multi-Terminal Test

Test performed:

- Opened both `spede-term com2` and `spede-term com3`.
- Logged into both terminals using valid credentials.
- Ran commands in both terminals.

Expected result:

- Each terminal works independently.
- Input/output from one terminal does not interfere with the other.
- Both remote terminals can be used at the same time.

Actual result:

- COM2 and COM3 both accepted login.
- Commands could be run independently in both serial terminals.
- Input/output from one terminal did not interfere with the other.

## Backspace/Delete Test

Test performed:

```text
type: hela
press Backspace/Delete
type: p
```

Expected result:

```text
help
```

Additional expected behavior:

- Backspace/Delete removes characters from the current input line.
- Backspace/Delete does not erase the `spede>` prompt.
- Backspace/Delete does not corrupt the submitted command buffer.

Actual result:

- Backspace/Delete correctly removed characters from the current input line.
- The `spede>` prompt was not erased.
- Corrected commands executed as expected.

## Serial Echo Test

Test performed:

- Typed normal characters in the remote terminal.
- Used Backspace/Delete while editing input.
- Pressed Enter after completing commands.

Expected result:

- Normal typed characters echo once.
- Backspace/Delete visually removes the previous input character.
- Pressing Enter moves to a new line cleanly.
- No duplicated or garbled characters appear.

Actual result:

- Characters echoed correctly.
- Backspace/Delete updated the terminal display correctly.
- Enter produced clean command submission behavior.

## Existing Phase 5 Regression Tests

Existing functionality checked:

- VGA/TTY shell processes still start.
- Ping/pong processes continue running.
- Semaphore wait/post behavior continues without freezing.
- Mutex shell lock behavior still works.
- Recursive mutex locking around shell input does not deadlock.

Expected result:

- Existing Phase 5 behavior continues working after Phase 5B changes.
- Ping/pong output does not freeze.
- Shells remain usable.
- Mutex and semaphore behavior remains stable.

Actual result:

- Shell processes still started.
- Ping/pong processes continued running.
- Semaphore wait/post logs showed processes blocking and waking.
- Mutex shell locking continued to work.
- Recursive shell input locking did not deadlock.