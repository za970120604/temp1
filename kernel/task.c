#include "header/task.h"

task_t* task_head;
int cur_prio = LOW_PRIO;

void init_task_queue() {
    disable_interrupt();
    task_head = (task_t*)task_malloc(sizeof(task_t));
    task_t* task_end = (task_t*)task_malloc(sizeof(task_t));

    task_head->prio = -1;  
    task_head->prev = NULL;
    task_head->next = task_end;

    task_end->prio = LOW_PRIO; 
    task_end->prev = task_head;
    task_end->next = NULL;
    enable_interrupt();
}

void insert_task(task_t* new_task) {
    task_t* iter = task_head;
    
    while (iter->prio < LOW_PRIO){
        if (new_task->prio < iter->prio)
            break;
        iter = iter->next;
    }

    new_task->next = iter;
    new_task->prev = iter->prev;
    
    iter->prev = new_task;
    new_task->prev->next = new_task;
}

void add_task(task_callback_t callback, int prio) {
    disable_interrupt(); 
    task_t* new_task = (task_t*)task_malloc(sizeof(task_t));
    if (new_task == (task_t*)NULL) {
    	uart_send_string("TASK BUFFER OOOOOOOOM\r\n");
    	return;
    }

    new_task->prio = prio;
    new_task->callback = callback; 

    insert_task(new_task);  

    enable_interrupt();  
}


void pop_task() {
    disable_interrupt();  
    task_t* exec_task = task_head->next;

    if (exec_task->prio >= cur_prio)
        return;

    // 保存原來的優先級
    int orin_prio = cur_prio;

    task_head->next = exec_task->next;
    task_head->next->prev = task_head;
    
    cur_prio = exec_task->prio;

    enable_interrupt(); 

    exec_task->callback();

    disable_interrupt(); 
    cur_prio = orin_prio;
    enable_interrupt();
}
