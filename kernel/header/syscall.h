#ifndef SYSCALL_H
#define SYSCALL_H

#include "sched.h"
#include "uart.h"
#include "exception.h"
#include "mailbox.h"
#include "cpio.h"
#include "utils.h"
#include "thread.h"

int syscall_getpid();
unsigned int syscall_uart_read(char buf[], unsigned int) ;
unsigned int syscall_uart_write(const char buf[], unsigned int);
int syscall_exec(const char* , char* const argv[]);
void syscall_exit();
int syscall_mailbox_call(unsigned char, unsigned int*);
void syscall_kill(int);
int syscall_fork(trap_frame_t*);
int copy_thread(trap_frame_t*);
extern void _child_fork_ret();

#endif // SYSCALL_H