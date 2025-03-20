#ifndef _TIMER_H
#define _TIMER_H

#include "utils.h"
#include "exception.h"

typedef struct {
    void (*func_with_arg)(char*);
    void (*func)();
    unsigned long expire_time;
    char* arg;
    int with_arg;
    int available;
} timer_t;



void init_timer_tcb_queue();
unsigned long get_current_time();

void add_timer(void (*callback)(), void* , unsigned long );
void set_timeout(char* , unsigned long );
void sleep(void (*wakeup_func)(), unsigned long );
void print_current_time();

void os_time_tick();
void core_timer_handler();
void two_sec_timer_handler();
void test_timer();
void test_preempt();
#endif