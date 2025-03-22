#ifndef _TASK_H
#define _TASK_H

#include "uart.h"
#include "allocator.h"
#include "buddy.h"
#include "utils.h"
#include "exception.h"

#define LOW_PRIO   0x100
#define TIMER_PRIO 0x010
#define UART_PRIO  0x001

typedef void (*task_callback_t)(void);

typedef struct task_struct task_t;

struct task_struct {
    task_t* prev;             
    task_t* next;             
    int prio;                 
    task_callback_t callback; 
};


void init_task_queue(void);
void add_task(task_callback_t callback, int prio);
void pop_task(void);


void insert_task(task_t* new_task);

#endif // _TASK_H
