#ifndef THREAD_H
#define THREAD_H

#define MAX_THREAD_COUNT 32768
#define THREAD_FREE 0
#define THREAD_READY 1
#define THREAD_RUN 2
#define THREAD_DEAD 3

#define KERNEL_STK_SIZE 4096
#define USER_STK_SIZE 4096
#define THREAD_NULL ((thread_t*)0)

#include "buddy.h"
#include "uart.h"

// 先完整定義thread_t結構體
typedef struct thread_callee_saved_ctx {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;
    unsigned long lr;
    unsigned long sp;
} thread_ctx_t;

typedef struct thread {
    unsigned int id;
    int status;
    thread_ctx_t ctx;
    unsigned char* user_sp;
    unsigned char* kernel_sp;
    struct thread* prev;    
    struct thread* next;
} thread_t;

// 函數聲明
extern thread_t* thread_descriptors;
extern thread_t* cur_thread;

void init_threads();
thread_t* thread_create(void (*)(void));
void thread_exit();
void idle();
void kill_zombies();
void set_ctx(unsigned long);

#endif // THREAD_H