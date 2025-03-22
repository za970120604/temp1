#include "header/allocator.h"
#include "header/utils.h"
#include "header/uart.h"
unsigned char TASK_MEM[MAX_SIMPLE_MALLOC_SIZE];
unsigned char* curPtr_task = TASK_MEM;
unsigned char* endPtr_task = TASK_MEM + MAX_SIMPLE_MALLOC_SIZE;

unsigned char USER_MEM[MAX_SIMPLE_MALLOC_SIZE];
unsigned char* curPtr_user = USER_MEM;
unsigned char* endPtr_user = USER_MEM + MAX_SIMPLE_MALLOC_SIZE; 

void* task_malloc(unsigned int size) {
    if (curPtr_task + size > endPtr_task) { 
    	uart_send_string("TASK BUFFER OOM\r\n");
        return NULL;  
    }

    void* allocated_mem = curPtr_task;
    curPtr_task += size; 

    return allocated_mem;
}

void* user_malloc(unsigned int size) {
    if (curPtr_user + size > endPtr_user) { 
    	uart_send_string("USER BUFFER OOM\r\n");
        return NULL;  
    }

    void* allocated_mem = curPtr_user;
    curPtr_user += size; 

    return allocated_mem;
}
