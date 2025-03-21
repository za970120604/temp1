#ifndef SCHED_H
#define SCHED_H

struct thread;
typedef struct thread thread_t;

#include "shell.h"
#include "timer.h"

extern thread_t* RQ_head;
extern thread_t* RQ_tail;
extern thread_t* cur_thread;

void init_scheduler();
void schedule_timer();
void schedule();
void push_rq(thread_t*);
thread_t* pop_rq();
void foo();

#endif // SCHED_H
