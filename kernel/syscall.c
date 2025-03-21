#include "header/syscall.h"

int syscall_getpid() {
    return cur_thread->id;
}

unsigned int syscall_uart_read(char buf[], unsigned int size) {
    unsigned int count = 0;
    for (; count < size; count++) {
        buf[count] = uart_recv_char();
    }
    return count;
}

unsigned int syscall_uart_write(const char buf[], unsigned int size) {
    unsigned int count = 0;
    for (; count < size; count++) {
        uart_send_char(buf[count]);
    }
    return count;
}

int syscall_exec(const char* name, char* const argv[]){
    char* file_content = find_file(name);
    unsigned int file_size = get_cpio_file_size(name);

    
    disable_interrupt();
    cur_thread->prog_size = file_size;
    utils_memcpy(file_content, cur_thread->prog, file_size);
    enable_interrupt();

    return 0;
}


void syscall_exit(){
    disable_interrupt();
    cur_thread->status = THREAD_DEAD;
    enable_interrupt();
    schedule();
}

int syscall_mailbox_call(unsigned char channel, unsigned int* mailbox){
    disable_interrupt();

    unsigned int message = ((unsigned int)((unsigned long)mailbox) & ~0xF) | (channel & 0xF);

    while (*MAILBOX_STATUS_REG & MAILBOX_FULL) {
        asm volatile("nop");
    }

    *MAILBOX_WRITE_REG = message;

    while (1) {
        while (*MAILBOX_STATUS_REG & MAILBOX_EMPTY) {
            asm volatile("nop");
        }

        if (*MAILBOX_READ_REG == message) {
            enable_interrupt();
            return (mailbox[1] == MAILBOX_RESPONSE); // Verify if the response is valid
        }
    }

    enable_interrupt();
    return 0;
}

void syscall_kill(int thread_id){
    disable_interrupt();

    if (thread_id < 0 || thread_id >= MAX_THREAD_COUNT || thread_descriptors[thread_id].status == THREAD_FREE){
        enable_interrupt();
        return;
    }

    thread_descriptors[thread_id].status = THREAD_DEAD;
    enable_interrupt();

    return;
}

int syscall_fork(trap_frame_t* tpf) {
    disable_interrupt();
    int child_pid = copy_thread(tpf);
    enable_interrupt();
    return child_pid;
}

int copy_thread(trap_frame_t* tpf) {
    thread_t* child_thread = thread_create(_child_fork_ret);
    if (child_thread == THREAD_NULL) {
        return -1;
    }
    
    // 手動分配用戶棧
    child_thread->user_sp = (unsigned char*)malloc(USER_STK_SIZE);
    if (child_thread->user_sp == NULL) {
        // 清理並返回錯誤
        child_thread->status = THREAD_FREE;
        free(child_thread->kernel_sp);
        return -1;
    }
    
    // 根據你的 memcpy 參數順序 (src, dst, len)
    utils_memcpy(cur_thread->user_sp, child_thread->user_sp, USER_STK_SIZE);
    utils_memcpy(cur_thread->kernel_sp, child_thread->kernel_sp, KERNEL_STK_SIZE);
    
    if (cur_thread->prog_size > 0) {
        child_thread->prog_size = cur_thread->prog_size;
        child_thread->prog = (unsigned char*)malloc(child_thread->prog_size);
        if (child_thread->prog == NULL) {
            free(child_thread->user_sp);
            child_thread->status = THREAD_FREE;
            return -1;
        }
        utils_memcpy(cur_thread->prog, child_thread->prog, cur_thread->prog_size);
    }
    
    // 計算子線程的陷阱框架位置
    trap_frame_t* child_tpf = (trap_frame_t*)((unsigned char*)tpf + 
                          ((unsigned char*)child_thread->kernel_sp - (unsigned char*)cur_thread->kernel_sp));
    
    // 設置子線程的上下文
    child_thread->ctx.sp = (unsigned long)child_tpf;
    
    // 設置子進程返回值為0
    child_tpf->x0 = 0;
    
    // 設置子進程的幀指針
    child_tpf->x29 = child_thread->ctx.fp;
    
    // 調整子進程的用戶棧指針
    child_tpf->sp_el0 = tpf->sp_el0 + 
                     (unsigned long)child_thread->user_sp - (unsigned long)cur_thread->user_sp;
    
    // 將子線程加入就緒隊列
    push_rq(child_thread);
    
    // 返回子進程ID
    return child_thread->id;
}

