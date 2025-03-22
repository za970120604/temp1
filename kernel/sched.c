#include "header/sched.h"
#include "header/thread.h"

thread_t* RQ_head = THREAD_NULL;
thread_t* RQ_tail = THREAD_NULL;
thread_t* cur_thread = THREAD_NULL;

void init_scheduler(){
    init_threads();
    thread_t* main_thread = thread_create(idle);
    set_ctx((unsigned long)&(main_thread->ctx));

    unsigned long main_stack;

    asm volatile(
        "mov    %0, sp"
        : "=r"(main_stack)
    );

    main_thread->kernel_sp = (unsigned char*)main_stack;
    cur_thread = main_thread;
    cur_thread->status = THREAD_RUN;
    
    thread_t* shell_thread = thread_create(shell);
    push_rq(shell_thread);
}

void schedule() {
    if (RQ_head == THREAD_NULL) return;
    
    // 保存當前線程
    thread_t* off_thread = cur_thread;
    
    // 找到下一個非死亡線程
    thread_t* target_thread;
    do {
        target_thread = pop_rq();
        if (target_thread->status == THREAD_DEAD) {
            push_rq(target_thread);
        }
    } while (target_thread->status == THREAD_DEAD);
    
    // 更新線程狀態
    cur_thread = target_thread;
    cur_thread->status = THREAD_RUN;
    
    // 處理舊線程
    if (off_thread->status != THREAD_DEAD) {
        off_thread->status = THREAD_READY;
    }
    push_rq(off_thread);
    
    // 執行上下文切換
    switch_to(get_current(), &(cur_thread->ctx));
}

// 將線程添加到就緒佇列尾部
void push_rq(thread_t* th) {
    th->next = THREAD_NULL;
    
    if (RQ_tail != THREAD_NULL) {
        // 佇列非空，添加到尾部
        RQ_tail->next = th;
        th->prev = RQ_tail;
        RQ_tail = th;
    } 
    else {
        // 佇列為空，初始化頭尾
        th->prev = THREAD_NULL;
        RQ_head = th;
        RQ_tail = th;
    }
}

// 從就緒佇列頭部取出線程
thread_t* pop_rq() {
    thread_t* target_thread = RQ_head;
    
    if (target_thread != THREAD_NULL) {
        // 更新佇列頭
        RQ_head = target_thread->next;
        
        // 更新新頭的prev指針或尾指針
        if (RQ_head != THREAD_NULL) {
            RQ_head->prev = THREAD_NULL;
        } else {
            RQ_tail = THREAD_NULL;  // 佇列變空
        }
        
        // 斷開返回線程與佇列的連接
        target_thread->next = THREAD_NULL;
    }
    
    return target_thread;
}

void foo() {
    char buffer[20]; // 用於整數轉字符串
    
    for(int i = 0; i < 10; ++i) {
        uart_send_string("\r\nThread ID: ");
        
        // 轉換線程ID為字符串
        utils_int_to_str(cur_thread->id, buffer);
        uart_send_string(buffer);
        
        uart_send_string(" ");
        
        // 轉換循環變量i為字符串
        utils_int_to_str(i, buffer);
        uart_send_string(buffer);

        for (int i = 0; i < 1000000; i++)
            asm volatile("nop");

        schedule();
    }
    thread_exit();
}

