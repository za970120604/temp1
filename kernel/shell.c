#include "header/uart.h"
#include "header/utils.h"
#include "header/reboot.h"
#include "header/mailbox.h"
#include "header/shell.h"
#include "header/cpio.h"
#include "header/allocator.h"
#include "header/timer.h"
#include "header/exception.h"
#include "header/buddy.h"
#include "header/thread.h"
extern void _enable_core_timer();
extern void _disable_core_timer();

void display_help() {
    uart_send_string("help   : print this help menu\n");
    uart_send_string("hello  : print Hello World!\n");
    uart_send_string("info   : get the hardware's information\n");
    uart_send_string("reboot : reboot the device\n");
    uart_send_string("ls     : list all files in cpio archive folder\n");
    uart_send_string("cat    : show specific file content\n");
    uart_send_string("malloc : get allocated free space starting address\n");
    uart_send_string("exec   : run the user program img if the provided img name exists in cpio archive\n");
    uart_send_string("async  : test asynchronous uart, it will asynchronously echos the user input\n");
    uart_send_string("timer  : test timer\n");
    uart_send_string("setTimeout : set timeout duration and the message after timeout\n");
    uart_send_string("preempt: test preemption\n");
    uart_send_string("buddy_pressure: write all non-reserved block with specific pattern\n");
    uart_send_string("buddy_check: check the pattern of all non-reserved block\n");
    uart_send_string("buddy_de_pressure: free all non-reserved block with specific pattern\n");
}

void process_command(char *input_string) {
    char *cmd = input_string;
    char *arg = NULL;

    // Find the first space in the input
    while (*input_string) {
        if (*input_string == ' ') {
            *input_string = '\0';  // Replace space with null to separate command and argument
            arg = input_string + 1; // The argument starts after the space
            break;
        }
        input_string++;
    }

    if (utils_string_compare(cmd, "help") > 0) {
        display_help();
    }
    else if (utils_string_compare(cmd, "hello") > 0) {
        uart_send_string("Hello World!\n");
    }
    else if (utils_string_compare(cmd, "info") > 0) {
        unsigned int board_revision = get_board_revision();
        if(board_revision != 0) {
            uart_send_string("My board model is: ");
            uart_binary_to_hex(board_revision); 
            uart_send_string("\r\n");
        }
        else {
            uart_send_string("Failed to get board model info.\r\n");
        }
        
        unsigned int base, size;
        if (get_arm_memory_info(&base, &size)) {
            uart_send_string("My ARM memory base address is: ");
            uart_binary_to_hex(base);
            uart_send_string("\r\n");
    
            uart_send_string("My ARM memory size is: ");
            uart_binary_to_hex(size);
            uart_send_string("\r\n");
        }
        else {
            uart_send_string("Failed to get ARM memory info.\r\n");
        }
    }
    else if (utils_string_compare(cmd, "reboot") > 0) {
        uart_send_string("Rebooting....\n");
        reset(1000);
    }
    else if (utils_string_compare(cmd, "ls") > 0) {
        cpio_ls();
    }
    else if (utils_string_compare(cmd, "cat") > 0) {
        if (arg) {
            cpio_cat(arg);
        } 
        else {
            uart_send_string("Usage: cat <filename>\n");
        }
    }
    else if (utils_string_compare(cmd, "malloc") > 0) {
        if (arg) { 
            unsigned int size_arg = utils_DecStr2Int(arg, utils_strlen(arg));
            void* start_address = user_malloc(size_arg);
            if (start_address) {
                uart_send_string("Start address: ");
                uart_binary_to_hex((unsigned int)((unsigned long)start_address));
                uart_send_string("\n");
                uart_send_string("Allocated size: ");
                uart_send_string(arg);
                uart_send_string("\n");
            }
            else{
                uart_send_string("Failed to allocate request memory.\r\n");
            }
        } 
        else {
            uart_send_string("Usage: malloc <request size>\n");
        }
    }
    else if (utils_string_compare(cmd, "exec") > 0) {
        if (arg) { 
            cpio_exec(arg);
        } 
        else {
            uart_send_string("Usage: exec <program_name.img>\n");
        }
    }
    else if (utils_string_compare(cmd, "async") > 0) {
        uart_send_string("Async Enter: ");
        test_async_uart();
    }
    else if (utils_string_compare(cmd, "timer") > 0) {
        test_timer();
    }
    else if (utils_string_compare(cmd, "setTimeout") > 0) {
        if (arg) {
            char *msg = arg;
            char *sec = NULL;
            while (*msg) {
                if (*msg == ' ') {
                    *msg = '\n';  // Replace space with null to separate command and argument
                    sec = msg + 1; // The argument starts after the space
                    break;
                }
                msg++;
            }
            unsigned long sec_arg = utils_DecStr2Int(sec, utils_strlen(sec));
            *sec = '\0';
            uart_binary_to_hex(sec_arg);
            uart_send_string("\n");
            set_timeout(arg, sec_arg);
        } 
        else {
            uart_send_string("Usage: setTimeout <MESSAGE> <SECONDS>\n");
        }
    }
    else if (utils_string_compare(cmd, "preempt") > 0) {
        test_preempt();
    }
    else if (utils_string_compare(cmd, "buddy_pressure") > 0) {
        test_allocate_all_memory();
    }
    else if (utils_string_compare(cmd, "buddy_check") > 0) {
        memcheck();
    }
    else if (utils_string_compare(cmd, "buddy_de_pressure") > 0) {
        test_free_all_memory();
    }
    else if (utils_string_compare(cmd, "test_fork") > 0) {
        thread_exec(fork_test, 0);
    }
    else {
        uart_send_string("Unknown command. Type 'help' for available commands.\n");
    }
}

void fork_test(){
    char int_str[20]; // 用於整數轉字串
    
    uart_send_string("\nFork Test, pid ");
    utils_int_to_str(getpid(), int_str);
    uart_send_string(int_str);
    uart_send_string("\n");
    
    int cnt = 1;
    int ret = 0;
    
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        
        uart_send_string("\nFirst child pid: ");
        utils_int_to_str(getpid(), int_str);
        uart_send_string(int_str);
        
        uart_send_string(", cnt: ");
        utils_int_to_str(cnt, int_str);
        uart_send_string(int_str);
        
        uart_send_string(", ptr: 0x");
        uart_binary_to_hex((unsigned int)(unsigned long)&cnt);
        
        uart_send_string(", sp: 0x");
        uart_binary_to_hex((unsigned int)cur_sp);
        uart_send_string("\n");
        
        ++cnt;
        
        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            
            uart_send_string("\nFirst child pid: ");
            utils_int_to_str(getpid(), int_str);
            uart_send_string(int_str);
            
            uart_send_string(", cnt: ");
            utils_int_to_str(cnt, int_str);
            uart_send_string(int_str);
            
            uart_send_string(", ptr: 0x");
            uart_binary_to_hex((unsigned int)(unsigned long)&cnt);
            
            uart_send_string(", sp: 0x");
            uart_binary_to_hex((unsigned int)cur_sp);
            uart_send_string("\n");
            
            while(1){
                // 無限循環
            }
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                
                uart_send_string("\nSecond child pid: ");
                utils_int_to_str(getpid(), int_str);
                uart_send_string(int_str);
                
                uart_send_string(", cnt: ");
                utils_int_to_str(cnt, int_str);
                uart_send_string(int_str);
                
                uart_send_string(", ptr: 0x");
                uart_binary_to_hex((unsigned int)(unsigned long)&cnt);
                
                uart_send_string(", sp: 0x");
                uart_binary_to_hex((unsigned int)cur_sp);
                uart_send_string("\n");
                
                for (int i = 0; i < 1000000; i++) {
                    asm volatile("nop"); // 延遲
                }
                
                ++cnt;
            }
            while(1){
                // 無限循環
            }
        }
        exit();
    }
    else {
        uart_send_string("\nParent here, pid: ");
        utils_int_to_str(getpid(), int_str);
        uart_send_string(int_str);
        
        uart_send_string(", child: ");
        utils_int_to_str(ret, int_str);
        uart_send_string(int_str);
        uart_send_string("\n");
        
        while(1){
            // 無限循環
        }
    }
}


void shell() {
    char array_space[256];  
    char *input_string = array_space; 

    while (1) {
        char element;
        uart_send_string("# "); 

        while (1) {
            element = uart_recv_char();
            *input_string++ = element;  
            uart_send_char(element);   

            if (element == '\n') {
                *(input_string - 1) = '\0';  // Overwrite '\n' with '\0'
                break;
            }

            if (input_string >= &array_space[255]) {  
                uart_send_string("\nInput too long! Try again.\n");
                input_string = array_space; 
                break;
            }
        }

        input_string = array_space;
        process_command(input_string);
    }
}
