/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * User Programs
 */

#include <spede/stdio.h>
#include <spede/string.h>

#include "syscall.h"

#define BUF_SIZE 128

#define pprintf(fmt, ...) { \
    char __pprint_buf[512] = {0}; \
    int i = snprintf(__pprint_buf, sizeof(__pprint_buf), (fmt), ##__VA_ARGS__); \
    if (i > 0) { \
        io_write(PROC_IO_OUT, __pprint_buf, i); \
    } \
}

#define CMD_EXIT  "exit"
#define CMD_HELP  "help"
#define CMD_SLEEP "sleep"
#define CMD_TIME  "time"
#define CMD_LOCK  "lock"

/*
 * Mutexes for the lock command
 */
int shell_mutex[2] = {-1, -1};

void prog_shell(void) {
    char buf[BUF_SIZE];
    char name[32];
    char os_name[128];

    int buflen;
    int reading;

    int pid = proc_get_pid();

    if (shell_mutex[pid % 2] < 0) {
        shell_mutex[pid % 2] = mutex_init();
    }

    if (proc_get_name(name) != 0) {
        pprintf("error getting process name!");
        proc_exit(-1);
    }

    if (sys_get_name(os_name) != 0) {
        pprintf("error getting system name!");
        proc_exit(-1);
    }

    // Generate a sleep value based upon the process id
    int sleep_seconds = (1 + (pid % 4)) * 4;

    memset(buf, 0, BUF_SIZE);

    // Ensure that the input and output buffers are flushed
    io_flush(PROC_IO_IN);
    io_flush(PROC_IO_OUT);

    pprintf("%s %s (process id %d) is running!\n", os_name, name, pid);
    pprintf("Sleeping for %d seconds at time %d ... ", sleep_seconds, sys_get_time());

    proc_sleep(sleep_seconds);

    pprintf("... and awake at time %d!\n\n", sys_get_time());

    while (1) {
        char input[BUF_SIZE];
        int input_len = 0;

        memset(input, 0, sizeof(input));
        memset(buf, 0, sizeof(buf));

        pprintf("%s[%d]$ ", os_name, pid);

        reading = 1;

        while (reading) {
            mutex_lock(shell_mutex[pid % 2]);
            mutex_lock(shell_mutex[pid % 2]);

            buflen = io_read(PROC_IO_IN, buf, BUF_SIZE);

            for (int i = 0; i < buflen; i++) {
                if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == 0) {
                    char newline = '\n';

                    io_write(PROC_IO_OUT, &newline, 1);

                    input[input_len] = '\0';
                    reading = 0;
                    break;
                } else if (buf[i] == '\b' || buf[i] == 127) {
                    if (input_len > 0) {
                        char erase[] = "\b \b";

                        input_len--;
                        input[input_len] = '\0';

                        io_write(PROC_IO_OUT, erase, 3);
                    }
                } else {
                    if (input_len < BUF_SIZE - 1) {
                        input[input_len] = buf[i];
                        input_len++;
                        input[input_len] = '\0';

                        io_write(PROC_IO_OUT, &buf[i], 1);
                    }
                }
            }

            mutex_unlock(shell_mutex[pid % 2]);
            mutex_unlock(shell_mutex[pid % 2]);
        }

        if (input_len) {
            if (strncmp(input, CMD_HELP, strlen(CMD_HELP)) == 0) {
                pprintf("Enter one of the following commands:\n");
                pprintf("\texit\t  exits the process\n");
                pprintf("\tlock\t  takes a lock that may block other shells\n");
                pprintf("\tsleep\t  puts the process to sleep for %d seconds\n", sleep_seconds);
                pprintf("\ttime\t  displays the current system time\n");
                pprintf("\n");
            } else if (strncmp(input, CMD_SLEEP, strlen(CMD_SLEEP)) == 0) {
                pprintf("Sleeping for %d seconds at time %d ... ", sleep_seconds, sys_get_time());
                proc_sleep(sleep_seconds);
                pprintf("... and awake at time %d!\n", sys_get_time());
            } else if (strncmp(input, CMD_TIME, strlen(CMD_TIME)) == 0) {
                pprintf("The current time is %d seconds\n", sys_get_time());
            } else if (strncmp(input, CMD_EXIT, strlen(CMD_EXIT)) == 0) {
                pprintf("Exiting process id %d\n", pid);
                proc_exit(0);
            } else if (strncmp(input, CMD_LOCK, strlen(CMD_LOCK)) == 0) {
                pprintf("Locking shells for %d seconds\n", sleep_seconds);
                mutex_lock(shell_mutex[pid % 2]);
                proc_sleep(sleep_seconds);
                mutex_unlock(shell_mutex[pid % 2]);
            } else {
                pprintf("You entered the following:\n%s\n", input);
            }
        }
    }
}

/*
 * Semaphores used for the "pingpong" program
 */
int pingpong_semaphores[2] = {-1, -1};

void prog_ping(void) {
    int pid = proc_get_pid();

    int *ping = &pingpong_semaphores[0];
    int *pong = &pingpong_semaphores[1];

    if (*ping < 0) {
        *ping = sem_init(1);
    }

    if (*pong < 0) {
        *pong = sem_init(0);
    }

    sem_post(*pong);

    while (1) {
        sem_wait(*ping);
        pprintf("%04d pingpong[%02d] ping!\n", sys_get_time(), pid);
        proc_sleep((pid % 2) + 3);
        sem_post(*pong);
    }
}

void prog_pong(void) {
    int pid = proc_get_pid();

    int *ping = &pingpong_semaphores[0];
    int *pong = &pingpong_semaphores[1];

    if (*ping < 0) {
        *ping = sem_init(0);
    }

    if (*pong < 0) {
        *pong = sem_init(1);
    }

    while (1) {
        sem_wait(*pong);
        pprintf("%04d pingpong[%02d] pong!\n", sys_get_time(), pid);
        proc_sleep((pid % 2) + 2);
        sem_post(*ping);
    }
}

/**
 * Remote serial terminal program.
 */
void remote_term(void) {
    char username[BUF_SIZE];
    char password[BUF_SIZE];
    char command[BUF_SIZE];
    char msg[256];

    int port_num = PortAlloc();

    if (port_num < 0) {
        proc_exit(-1);
    }

    while (1) {
        memset(username, 0, sizeof(username));
        memset(password, 0, sizeof(password));

        PortWrite("Now enter (username): ", port_num);
        PortRead(username, port_num);

        PortWrite("\r\nNow enter (password): ", port_num);
        PortRead(password, port_num);

        if (strcmp(username, "spede") == 0 &&
            strcmp(password, "spede") == 0) {
            break;
        }

        PortWrite("\r\nInvalid username or password.\r\n\r\n", port_num);
    }

    PortWrite("\r\nHello, World! Team MyOS here!\r\n", port_num);

    while (1) {
        memset(command, 0, sizeof(command));

        PortWrite("spede> ", port_num);
        PortRead(command, port_num);

        PortWrite("\r\n", port_num);

        if (strcmp(command, "help") == 0) {
            PortWrite("Enter one of the following commands:\r\n", port_num);
            PortWrite("exit exits the process\r\n", port_num);
            PortWrite("sleep puts the process to sleep for 10 seconds\r\n", port_num);
            PortWrite("time displays the current system time\r\n", port_num);
        } else if (strcmp(command, "time") == 0) {
            memset(msg, 0, sizeof(msg));
            snprintf(msg, sizeof(msg), "The current time is %d seconds\r\n", sys_get_time());
            PortWrite(msg, port_num);
        } else if (strcmp(command, "sleep") == 0) {
            PortWrite("Sleeping for 10 seconds...\r\n", port_num);
            proc_sleep(10);
            PortWrite("Awake.\r\n", port_num);
        } else if (strcmp(command, "exit") == 0) {
            PortWrite("Exiting remote terminal.\r\n", port_num);
            proc_exit(0);
        } else if (command[0] != '\0') {
            PortWrite("Unknown command: ", port_num);
            PortWrite(command, port_num);
            PortWrite("\r\n", port_num);
        }
    }
}