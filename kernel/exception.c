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
            int cur_pid = syscall_getpid();
            tpf->x0 = cur_pid;
            break;
        case 1: 
            enable_interrupt();
            unsigned int read_len = syscall_uart_read(tpf->x0, tpf->x1);
            tpf->x0 = read_len;
            disable_interrupt();
            break;
        case 2:
            unsigned int write_len = syscall_uart_write(tpf->x0, tpf->x1);
            tpf->x0 = write_len;
            break;
        case 3:
            int exec_state = syscall_exec(tpf->x0, tpf->x1);
            tpf->elr_el1 = cur_thread->prog;
            tpf->sp_el0 = cur_thread->user_sp + USER_STK_SIZE;
            tpf->x0 = exec_state;
            break;
        case 4:
            int pid = syscall_fork(tpf);
            tpf->x0 = pid;
            break;
        case 5:
            syscall_exit();
            break;
        case 6:
            int mailbox_state = syscall_mailbox_call(tpf->x0, tpf->x1);
            tpf->x0 = mailbox_state;
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

void exec_in_el0(char* prog_head, unsigned int sp_loc){
    asm volatile(
        "mrs    x21, spsr_el1;"
        "mrs    x22, elr_el1;"
        "msr    spsr_el1, xzr;"
        "msr    elr_el1, %0;"
        "msr    sp_el0, %1;"
        "eret;"
        :: 
        "r"(prog_head),
        "r"(sp_loc)
    );
}


