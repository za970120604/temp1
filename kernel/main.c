#include "header/uart.h"
#include "header/shell.h"
#include "header/fdt.h"
#include "header/cpio.h"
#include "header/exception.h"
#include "header/buddy.h"
#include "header/thread.h"
#include "header/sched.h"
#include "header/timer.h"
extern void _enable_core_timer();
extern void _disable_core_timer();

int main() {
    uart_init();
    vbar_el1_logger_entry();
    cpio_addr = fdt_traverse(initramfs_callback, "chosen", "linux,initrd-start");
    if(cpio_addr == NULL){
        uart_send_string("Do not find target device!\r\n");
    }
    mem_init();
    
    enable_interrupt();
    enable_uart_interrupt();
    _enable_core_timer();

    unsigned long tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
    
    init_timer_tcb_queue();
    init_task_queue();
    init_async_buffer();
    

    // uart_send_string("Hello, world!\n");
    
    // Check current EL
    unsigned long el;
    asm volatile ("mrs %0, CurrentEL" : "=r" (el));
    el = (el >> 2) & 0x3;  // Extract EL level
    // uart_send_string("Current EL: ");
    // uart_binary_to_hex(el);
    // uart_send_string("\r\n");

    init_scheduler();
    // for(int i = 0; i < 5; ++i) {
        // thread_create(foo);
    // }
    disable_interrupt();
    scheduling_enabled = 1;
    enable_interrupt();
    
    // idle();
    shell();
    return 0;
}
