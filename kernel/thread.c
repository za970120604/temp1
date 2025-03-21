#include "header/thread.h"
#include "header/sched.h"
#include "header/exception.h"
thread_t* thread_descriptors;

void init_threads() {
    thread_descriptors = (thread_t*)malloc(sizeof(thread_t) * MAX_THREAD_COUNT);
    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        thread_descriptors[i].id = i;
        thread_descriptors[i].status = THREAD_FREE;  // 標記為空閒
        thread_descriptors[i].prev = THREAD_NULL;
        thread_descriptors[i].next = THREAD_NULL;
        thread_descriptors[i].prog_size = 0;
    }
}

thread_t* thread_create(void (*func)(void)) {
    // 1. 查找空閒線程描述符
    thread_t* new_thread = THREAD_NULL;
    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        if (thread_descriptors[i].status == THREAD_FREE) {
            new_thread = &thread_descriptors[i];
            break;
        }
    }
    
    if (new_thread == THREAD_NULL) {
        uart_send_string("Error: No free thread slots available\r\n");
        return THREAD_NULL;
    }
    
    
    unsigned char* kernel_stk = (unsigned char*)malloc(KERNEL_STK_SIZE);
    if (!kernel_stk) {
        uart_send_string("Error: Failed to allocate stack for thread\r\n");
        return THREAD_NULL;
    }
    unsigned long kernel_stk_top = (unsigned long)(kernel_stk + KERNEL_STK_SIZE);

    unsigned char* user_stk = (unsigned char*)malloc(USER_STK_SIZE);
    if (!user_stk) {
        uart_send_string("Error: Failed to allocate stack for thread\r\n");
        return THREAD_NULL;
    }
    unsigned long user_stk_top = (unsigned long)(user_stk + USER_STK_SIZE);
    
    // 初始化上下文
    new_thread->ctx.sp = kernel_stk_top;          // 堆棧頂
    new_thread->ctx.fp = kernel_stk_top;          // 框架指針初始與堆棧頂相同
    new_thread->ctx.lr = (unsigned long)func; // 設置函數入口點
    
    // 將其他callee-saved寄存器初始化為0
    new_thread->ctx.x19 = 0;
    new_thread->ctx.x20 = 0;
    new_thread->ctx.x21 = 0;
    new_thread->ctx.x22 = 0;
    new_thread->ctx.x23 = 0;
    new_thread->ctx.x24 = 0;
    new_thread->ctx.x25 = 0;
    new_thread->ctx.x26 = 0;
    new_thread->ctx.x27 = 0;
    new_thread->ctx.x28 = 0;
    
    // 4. 更新線程信息
    new_thread->kernel_sp = kernel_stk_top;           // 保存堆棧指針以便後續釋放
    new_thread->user_sp = user_stk_top;
    new_thread->status = THREAD_READY;       // 設置為就緒狀態
    new_thread->next = THREAD_NULL;
    new_thread->prev = THREAD_NULL;
    
    // 5. 將線程加入就緒隊列
    push_rq(new_thread);
    uart_send_string("Thread created with ID: ");
    char id_str[10];
    utils_int_to_str(new_thread->id, id_str);
    uart_send_string(id_str);
    uart_send_string("\r\n");
    return new_thread;
}

void thread_exit(){
    disable_interrupt();
    cur_thread->status = THREAD_DEAD;
    enable_interrupt();
    schedule();
}

void idle(){
    while (1){
        kill_zombies();
        schedule();
    }
}

void kill_zombies() {
    disable_interrupt();
    
    thread_t* curr = RQ_head;
    thread_t* next;
    
    // 使用預先保存下一個節點的方式遍歷
    while (curr != THREAD_NULL) {
        next = curr->next;  // 先保存下一個節點，避免刪除當前節點後迭代器失效
        
        if (curr->status == THREAD_DEAD) {
            // 1. 從隊列中移除節點
            if (curr == RQ_head) {
                RQ_head = curr->next;
            } 
            else {
                curr->prev->next = curr->next;
            }
            
            if (curr == RQ_tail) {
                RQ_tail = curr->prev;
            } 
            else if (curr->next != THREAD_NULL) {
                curr->next->prev = curr->prev;
            }
            
            // 2. 釋放資源
            free(curr->kernel_sp);
            free(curr->user_sp);
            if (curr->prog) {
                free(curr->prog);
            }
            
            // 3. 將線程標記為可重用
            curr->status = THREAD_FREE;
            curr->next = THREAD_NULL;
            curr->prev = THREAD_NULL;
            curr->prog_size = 0;
            curr->prog = NULL;
        }
        
        curr = next;  // 移動到預先保存的下一個節點
    }
    
    enable_interrupt();
}

void set_ctx(unsigned long ctx_addr){
    asm volatile(
        "msr tpidr_el1, %0;" :: "r"(ctx_addr)
    );
}

void user_thread_run() {
    disable_interrupt();
    
    // 準備切換到 EL0
    asm volatile(
        "msr    spsr_el1, xzr;"        // 設置 SPSR_EL1 為零 (EL0t)
        "msr    elr_el1, x19;"         // 設置 ELR_EL1 為程序入口點 (x19)
        "msr    sp_el0, %0;"           // 設置 SP_EL0 為用戶棧頂
        ::  
        "r"(cur_thread->user_sp + USER_STK_SIZE)
    );
    
    enable_interrupt();
    
    // 執行 eret 指令，切換到 EL0 執行用戶程序
    asm volatile("eret;");
}

thread_t* user_thread_create(void(*func)(void)){
    thread_t* new_thread = thread_create(user_thread_run);
    if (new_thread == THREAD_NULL) {
        return THREAD_NULL;
    }

    new_thread->ctx.x19 = (unsigned long)func;
    return new_thread;
}
