#include "header/cpio.h"
#include "header/uart.h"
#include "header/utils.h"
void* cpio_addr = 0;
/*The header is followed by  the pathname of the entry (the length of the pathname is stored in the header)
and any file data.  The end of the archive is indicated	by a special
record with the pathname "TRAILER!!!".*/

char* find_file(char *name) {
    char *addr = (char *) cpio_addr;
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0) {
        struct cpio_header *header = (struct cpio_header *) addr;
        unsigned long filename_size = utils_HexStr2Int(header->c_namesize, sizeof(header->c_namesize));
        unsigned long file_size = utils_HexStr2Int(header->c_filesize, sizeof(header->c_filesize));
        unsigned long header_pathname_size = sizeof(struct cpio_header) + filename_size;

        header_pathname_size = utils_align(header_pathname_size, 4);
        file_size = utils_align(file_size, 4);

        if (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), name) != 0) {
            return addr;
        }
        addr += (header_pathname_size + file_size);
    }
    return NULL;
}


void cpio_ls() {
    char *addr = (char *) cpio_addr;
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0) {
        struct cpio_header *header = (struct cpio_header *) addr;
        unsigned long filename_size = utils_HexStr2Int(header->c_namesize, sizeof(header->c_namesize));
        unsigned long file_size = utils_HexStr2Int(header->c_filesize, sizeof(header->c_filesize));
        unsigned long header_pathname_size = sizeof(struct cpio_header) + filename_size;

        header_pathname_size = utils_align(header_pathname_size, 4);
        file_size = utils_align(file_size, 4);

        uart_send_string(addr + sizeof(struct cpio_header));
        uart_send_string("\r\n");

        addr += (header_pathname_size + file_size);
    }
}

void cpio_cat(char *filename) {
    char *target = find_file(filename);
    if (target) {
        struct cpio_header *header = (struct cpio_header *) target;
        unsigned long filename_size = utils_HexStr2Int(header->c_namesize, sizeof(header->c_namesize));
        unsigned long file_size = utils_HexStr2Int(header->c_filesize, sizeof(header->c_filesize));
        unsigned long header_pathname_size = sizeof(struct cpio_header) + filename_size;

        header_pathname_size = utils_align(header_pathname_size, 4);
        file_size = utils_align(file_size, 4);

        char *file_content = target + header_pathname_size;
        for (unsigned int i = 0; i < file_size; i++) {
            uart_send_char(file_content[i]);
        }
        // uart_send_string("\n");
    } 
    else {
        uart_send_string("File not found\r\n");
    }
}

void cpio_exec(char *filename) {
    char *target = find_file(filename);
    if (target) {
        struct cpio_header *header = (struct cpio_header *) target;
        unsigned long filename_size = utils_HexStr2Int(header->c_namesize, sizeof(header->c_namesize));
        unsigned long file_size = utils_HexStr2Int(header->c_filesize, sizeof(header->c_filesize));
        unsigned long header_pathname_size = sizeof(struct cpio_header) + filename_size;

        header_pathname_size = utils_align(header_pathname_size, 4);
        file_size = utils_align(file_size, 4);

        char *file_content = target + header_pathname_size;

        uart_send_string("Found and running ");
        uart_send_string(filename);
        uart_send_string("\r\n");

        // Set up EL0 execution
        asm volatile ("mov x0, 0"); // Enable interrupt in EL0
        asm volatile ("msr spsr_el1, x0");
        asm volatile ("msr elr_el1, %0" :: "r" (file_content)); // Set execution start point
        asm volatile ("mov x0, 0x20000"); // Set stack pointer for EL0
        asm volatile ("msr sp_el0, x0");
        asm volatile ("eret"); // Return to EL0 and execute
    } 
    else {
        uart_send_string("File not found\r\n");
    }
}

unsigned long get_cpio_end_address() {
    char *addr = (char *)cpio_addr;
    
    // 遍歷 CPIO 檔案直到找到 TRAILER!!! 標記
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0) {
        struct cpio_header *header = (struct cpio_header *)addr;
        unsigned long filename_size = utils_HexStr2Int(header->c_namesize, sizeof(header->c_namesize));
        unsigned long file_size = utils_HexStr2Int(header->c_filesize, sizeof(header->c_filesize));
        unsigned long header_pathname_size = sizeof(struct cpio_header) + filename_size;
        
        header_pathname_size = utils_align(header_pathname_size, 4);
        file_size = utils_align(file_size, 4);
        
        addr += (header_pathname_size + file_size);
    }
    
    // 處理 TRAILER!!! 記錄本身
    struct cpio_header *trailer_header = (struct cpio_header *)addr;
    unsigned long trailer_filename_size = utils_HexStr2Int(trailer_header->c_namesize, sizeof(trailer_header->c_namesize));
    unsigned long trailer_file_size = utils_HexStr2Int(trailer_header->c_filesize, sizeof(trailer_header->c_filesize));
    unsigned long trailer_header_pathname_size = sizeof(struct cpio_header) + trailer_filename_size;
    
    trailer_header_pathname_size = utils_align(trailer_header_pathname_size, 4);
    trailer_file_size = utils_align(trailer_file_size, 4);
    
    // 計算終止位置並轉換為 unsigned long
    addr += (trailer_header_pathname_size + trailer_file_size);
    
    // 使用類型轉換將指針轉為 unsigned long
    return (unsigned long)addr;
}

unsigned int get_cpio_file_size(char* name){

    char *addr = (char *) cpio_addr;
    while (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), "TRAILER!!!") == 0) {
        struct cpio_header *header = (struct cpio_header *) addr;
        unsigned long filename_size = utils_HexStr2Int(header->c_namesize, sizeof(header->c_namesize));
        unsigned long original_file_size = utils_HexStr2Int(header->c_filesize, sizeof(header->c_filesize));
        unsigned long header_pathname_size = sizeof(struct cpio_header) + filename_size;

        header_pathname_size = utils_align(header_pathname_size, 4);
        unsigned long file_size = utils_align(original_file_size, 4);

        if (utils_string_compare((char *)(addr + sizeof(struct cpio_header)), name) != 0) {
            return (unsigned int)original_file_size;
        }
        addr += (header_pathname_size + file_size);
    }

    return 0;
}
