#include "header/exception.h"
extern void _enable_core_timer();
extern void _disable_core_timer();
int nested_interrupt_cnt = 0;

void naive_irq_entry () {
}

void vbar_el1_logger_entry () {
    unsigned long vbar_el1;
    asm volatile ("mrs %0, vbar_el1" : "=r"(vbar_el1));  // Read VBAR_EL1
    uart_send_string("VBAR_EL1 is set to: ");
	uart_binary_to_hex((unsigned int)vbar_el1);
	uart_send_string("\r\n");
}

void el1_irq_entry () {
    if ((*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT) && (*CORE0_IRQ_SOURCE & IRQ_SOURCE_GPU)){
        disable_uart_interrupt();
        add_task(async_uart_handler_c, UART_PRIO);
        pop_task();
        enable_uart_interrupt();
    }
	else if (*CORE0_IRQ_SOURCE & IRQ_SOURCE_CNTPNSIRQ){
        _disable_core_timer();
        add_task(core_timer_handler, TIMER_PRIO);
        pop_task();
        _enable_core_timer();
    }
}

void el0_irq_entry () {
    if (*CORE0_IRQ_SOURCE & IRQ_SOURCE_CNTPNSIRQ){
        _disable_core_timer();
        add_task(two_sec_timer_handler, TIMER_PRIO);
        pop_task();
        // enable_core_timer();
        // two_sec_timer_handler();
    }
}

void enable_interrupt() {
    if (nested_interrupt_cnt > 0) {
        nested_interrupt_cnt--;
    }

    if (nested_interrupt_cnt == 0) {
        asm volatile("msr DAIFClr, 0xf;");
    }
}

void disable_interrupt() {
    if (nested_interrupt_cnt == 0) {
        asm volatile("msr DAIFSet, 0xf;");
    }   
    
    nested_interrupt_cnt++;
}


