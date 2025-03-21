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
        add_task(core_timer_handler, TIMER_PRIO);
        pop_task();
        _enable_core_timer();
    }
}

void el0_sync_entry(trap_frame_t* tpf) {
    disable_interrupt();
    unsigned long syscall_number = tpf->x8;
    
    switch(syscall_number) {
        case 0: 
            tpf->x0 = syscall_getpid();
            break;
        case 1: 
            enable_interrupt();
            tpf->x0 = syscall_uart_read(tpf->x0, tpf->x1);
            disable_interrupt();
            break;
        case 2:
            tpf->x0 = syscall_uart_write(tpf->x0, tpf->x1);
            break;
        case 3:
            int state = syscall_exec(tpf->x0, tpf->x1);
            tpf->elr_el1 = cur_thread->prog;
            tpf->sp_el0 = cur_thread->user_sp + USER_STK_SIZE;
            tpf->x0 = state;
            break;
        case 4:
            tpf->x0 = syscall_fork(tpf);
            break;
        case 5:
            syscall_exit();
            break;
        case 6:
            tpf->x0 = syscall_mailbox_call(tpf->x0, tpf->x1);
            break;
        case 7:
            syscall_kill(tpf->x0);
            break;
        default:
            uart_send_string("Unknown syscall number\r\n");
            break;
    }
    
    enable_interrupt();
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


