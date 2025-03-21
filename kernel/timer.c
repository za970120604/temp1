#include "header/timer.h"
extern void _enable_core_timer();
extern void _disable_core_timer();
volatile int scheduling_enabled = 0;

timer_t timer_tcb_queue[10];

void init_timer_tcb_queue () {
    for (int i = 0; i < 10; i++) {
        timer_tcb_queue[i].available = 1;
        timer_tcb_queue[i].with_arg = 0;
    }
}

void add_timer(void (*callback)(), void* arg, unsigned long expire_time) {
    disable_interrupt();

    for (int i = 0; i < 10; i++) {
        if (timer_tcb_queue[i].available == 1) {
            timer_tcb_queue[i].available = 0;
            timer_tcb_queue[i].expire_time = expire_time;

            if (arg) {
                timer_tcb_queue[i].func_with_arg = (void (*)(char*))callback;
                timer_tcb_queue[i].arg = (char*)arg;
                timer_tcb_queue[i].with_arg = 1;
            } 
            else {
                timer_tcb_queue[i].func = callback;
                timer_tcb_queue[i].with_arg = 0;
            }

            break;
        }
    }

    enable_interrupt();
}


void set_timeout(char* msg, unsigned long expire_time) {
    add_timer((void (*)())async_uart_puts, msg, expire_time);
}

void sleep(void (*wakeup_func)(), unsigned long expire_time) {
    add_timer((void (*)())wakeup_func, NULL, expire_time);
}

unsigned long get_current_time(){
    unsigned long long freq;
    asm volatile("mrs %0, cntfrq_el0;" :"=r"(freq));

    unsigned long long count;
    asm volatile("mrs %0, cntpct_el0;" :"=r"(count));

    unsigned long current_time = count / freq;
    return current_time;
}

void print_current_time() { 
    unsigned long current_time = get_current_time();
    uart_send_string("Current Time (after booting): ");
    uart_binary_to_hex((unsigned int)current_time);
    uart_send_string(" secs\n");
}

void os_time_tick () {
    disable_interrupt();
    for (int i = 0; i < 10; i++) {
        if (!timer_tcb_queue[i].available){
            timer_tcb_queue[i].expire_time--;

            if (timer_tcb_queue[i].expire_time <= 0){
                enable_interrupt();
                if (timer_tcb_queue[i].with_arg){
                    timer_tcb_queue[i].func_with_arg(timer_tcb_queue[i].arg);
                }
                else{
                    timer_tcb_queue[i].func();
                }
                disable_interrupt();
                timer_tcb_queue[i].available = 1;
            }
        }
    }
    
    if (scheduling_enabled) {
        enable_interrupt();
        schedule();
        disable_interrupt();
    }
    enable_interrupt();
}

void core_timer_handler() {
    os_time_tick();
    asm volatile(
        "mrs x0, cntfrq_el0;"  // 獲取頻率
        "lsr x0, x0, #5;"      // 頻率右移5位 (cntfrq_el0 >> 5)
        "msr cntp_tval_el0, x0;" // 設置新的過期時間
    );   
}

void two_sec_timer_handler() {
    print_current_time();
    asm volatile(
        "mov    x0, 1;"             
        "msr    cntp_ctl_el0, x0;"  // enable
        "mrs    x0, cntfrq_el0;"
        "add    x0, x0, x0;"
        "msr    cntp_tval_el0, x0;"
        "mov    x0, 2;"
        "ldr    x1, =0x40000040;"
        "str    w0, [x1];"          // unmask timer interrupt
    );
}

void test_timer() {
    uart_send_string("Starting timer test...\n");
    print_current_time();
    set_timeout("This message prints after 6 seconds.\n", 6);
    set_timeout("This message prints after 3 seconds.\n", 3);
    uart_send_string("Sleeping for 5 seconds...\n");
    sleep(print_current_time, 5);
    set_timeout("This message prints after 8 seconds.\n", 8);
}

void test_preempt() {
    add_task(print_current_time, 1);
    async_uart_puts("After Current Time\n");
    pop_task();
} 
