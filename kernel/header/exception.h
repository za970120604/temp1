#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include "utils.h"
#include "uart.h"
#include "timer.h"
#include "task.h"
#include "thread.h"
#include "syscall.h"

#define CORE0_TIMER_IRQ_CTRL        ((volatile unsigned int*)(0x40000040))
#define CORE0_IRQ_SOURCE            ((volatile unsigned int*)(0x40000060))
#define IRQ_SOURCE_GPU              (1 << 8)
#define IRQ_PENDING_1_AUX_INT       (1 << 29)
#define IRQ_SOURCE_CNTPNSIRQ        (1 << 1)

void naive_irq_entry();
void vbar_el1_logger_entry();
void el1_irq_entry();
void el0_irq_entry();
void el0_sync_entry(trap_frame_t* );

void enable_interrupt();
void disable_interrupt();

#endif