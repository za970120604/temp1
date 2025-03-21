#include "header/buddy.h"

// 全局變量宣告為指針，而不是數組
buddy_t* buddy_frame_array;  // 指向動態分配的內存幀描述符數組
buddy_t** free_frame_lists;  // 指向每個 order 的 free list 頭
unsigned int slab_sizes[SLAB_TYPE] = {16, 32, 48, 96, 128, 256, 512, 1024, 2048, 4096};
slab_t* slab_frame_array;    // 指向動態分配的 slab 幀描述符數組
slab_t** slab_start_list;    // 指向每種 slab 大小的開始列表

// heap 管理變量
extern char _kernel_start;
extern char _kernel_end;
extern char _heap_start;
extern char _heap_end;
void* heap_head;             // 當前 heap 頂部
void* heap_limit;            // heap 上限

unsigned int g_allocated_indices[MAX_ALLOCATED_BLOCKS];
int g_allocated_orders[MAX_ALLOCATED_BLOCKS];
int g_allocated_count = 0;
int g_test_pattern = 0xABCD1234;  // 識別模式

int size2order(unsigned int request_size) { // convert requested size to buddy order
    if (request_size == 0) {
        return 0;
    }
    unsigned int adjusted_size = (request_size + FRAME_SIZE - 1) / FRAME_SIZE;

    int order = 0;
    while ((unsigned int)(1 << order) < adjusted_size) {
        order++;
    }

    return order;
}

int is_block_free(int start, int size) { // is the starting block with the given buddy size a contiguious free block
    for (int i = start; i < start + size; i++) {
        if (buddy_frame_array[i].status == ALLOCATED_FRAME || 
            buddy_frame_array[i].status == RESERVED_FRAME) {
            return 0;
        }
    }
    return 1;
}

void buddy_init() {
    for (int i = 0; i <= MAX_ORDER; i++) {
        free_frame_lists[i] = BUDDY_NULL;  
    }

    for (int i = 0; i < TOTAL_FRAME; i++) {
        buddy_frame_array[i].index = i;
        if (buddy_frame_array[i].status != RESERVED_FRAME) {
            buddy_frame_array[i].status = BELONGED_FRAME;
        }
        buddy_frame_array[i].next = BUDDY_NULL;
        buddy_frame_array[i].prev = BUDDY_NULL;
        buddy_frame_array[i].allocated_order = -1;
    }


    int i = 0;
    while (i < TOTAL_FRAME) {
        if (buddy_frame_array[i].status == ALLOCATED_FRAME || 
            buddy_frame_array[i].status == RESERVED_FRAME) {
            i++;
            continue;
        }

        int max_order = MAX_ORDER;
        while ((i + (1 << max_order) > TOTAL_FRAME) ||  
               !is_block_free(i, 1 << max_order)) {   // construct a valid & largest buddy
            if (max_order == 0){ 
                break;
            }
            max_order--;
        }

        if (max_order >= 0) {
            buddy_frame_array[i].status = max_order;  // 設置為 free buddy
            
            // 加入對應的 free list
            buddy_frame_array[i].next = free_frame_lists[max_order];
            buddy_frame_array[i].prev = BUDDY_NULL;
            
            if (free_frame_lists[max_order] != BUDDY_NULL) {
                free_frame_lists[max_order]->prev = &buddy_frame_array[i];
            }
            
            free_frame_lists[max_order] = &buddy_frame_array[i];

            i += (1 << max_order);  // move to next possible start block of the buddy
        } 
        else {
            i++;
        }
    }
}

int buddy_allocate(int order) {
    int i;
    int split_count = 0;                 // 追踪分割次數
    int total_frames_released = 0;       // 追踪釋放的總帧數
    char int_str[20]; // 用於轉換整數到字符串
    
    for (i = order; i <= MAX_ORDER; i++) { // allocate as small buddy as possible
        if (free_frame_lists[i] != BUDDY_NULL) {
            buddy_t* block = free_frame_lists[i];
            unsigned int block_index = block->index;

            // uart_send_string("\r\nAllocating block of order ");
            // utils_int_to_str(order, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" at index ");
            // utils_int_to_str(block_index, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" (address ");
            // uart_binary_to_hex(index2address(block_index));
            // uart_send_string(", size ");
            // utils_int_to_str(1 << order, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" frames, ");
            // utils_int_to_str((1 << order) * FRAME_SIZE, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" bytes)\r\n");
            
            // 從 free list 中移除
            free_frame_lists[i] = block->next;
            if (block->next != BUDDY_NULL) {
                block->next->prev = BUDDY_NULL;
            }

            int current_order = i;
            
            // 如果分配的區塊比請求的大，計算將釋放的冗餘內存
            if (current_order > order) {
                int redundant_frames = (1 << current_order) - (1 << order);
                // 第二個打印語句
                // uart_send_string("\r\nFound block of order ");
                // utils_int_to_str(current_order, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" (larger than requested order ");
                // utils_int_to_str(order, int_str);
                // uart_send_string(int_str);
                // uart_send_string(") - need splitting\r\n");
                
                // // 第三個打印語句
                // uart_send_string("\r\nWill release ");
                // utils_int_to_str(redundant_frames, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" frames (");
                // utils_int_to_str(redundant_frames * FRAME_SIZE, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" bytes) of redundant memory through splitting\r\n");
            }

            // 分割大塊為所需大小
            while (current_order > order) {
                current_order--;
                int buddy_index = block_index + (1 << current_order); 
                split_count++;
                
                buddy_frame_array[buddy_index].status = current_order;
                buddy_frame_array[buddy_index].next = free_frame_lists[current_order];
                buddy_frame_array[buddy_index].prev = BUDDY_NULL;
                
                if (free_frame_lists[current_order] != BUDDY_NULL) {
                    free_frame_lists[current_order]->prev = &buddy_frame_array[buddy_index];
                }
                
                free_frame_lists[current_order] = &buddy_frame_array[buddy_index];

                int released_frames = 1 << current_order;
                total_frames_released += released_frames;

                // 第四個打印語句
                // uart_send_string("\r\nSplit: buddy at index ");
                // utils_int_to_str(buddy_index, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" (address ");
                // uart_binary_to_hex(index2address(buddy_index));
                // uart_send_string(") of order ");
                // utils_int_to_str(current_order, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" added to free list\r\n");
            }

            buddy_frame_array[block_index].status = ALLOCATED_FRAME;
            buddy_frame_array[block_index].next = BUDDY_NULL;
            buddy_frame_array[block_index].prev = BUDDY_NULL;
            buddy_frame_array[block_index].allocated_order = order; 

            // 輸出分割統計
            if (split_count > 0) {
                // 第五個打印語句
                // uart_send_string("\r\nSplit summary: block of order ");
                // utils_int_to_str(i, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" split ");
                // utils_int_to_str(split_count, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" times to get order ");
                // utils_int_to_str(order, int_str);
                // uart_send_string(int_str);
                // uart_send_string("\r\n");
                
                // // 第六個打印語句
                // uart_send_string("Total memory released: ");
                // utils_int_to_str(total_frames_released, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" frames (");
                // utils_int_to_str(total_frames_released * FRAME_SIZE, int_str);
                // uart_send_string(int_str);
                // uart_send_string(" bytes)\r\n");
            }

            // 第七個打印語句
            // uart_send_string("\r\nBlock at index ");
            // utils_int_to_str(block_index, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" (address ");
            // uart_binary_to_hex(index2address(block_index));
            // uart_send_string(") of order ");
            // utils_int_to_str(order, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" allocated\r\n");
            
            return block_index; 
        }
    }

    // uart_send_string("\r\nAllocation failed: No block of sufficient size available for order ");
    // utils_int_to_str(order, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" request\r\n");
    return -1;
}

void buddy_free(int block_index) { // input: the memory block index to be freed & the order of this memory block, assume the input information is correct
    if (buddy_frame_array[block_index].status != ALLOCATED_FRAME) {
        // uart_send_string("Error: Attempting to free an unallocated block at index ");
        // char int_str[20];
        // utils_int_to_str(block_index, int_str);
        // uart_send_string(int_str);
        // uart_send_string("\r\n");
        return;
    }
    
    int order = buddy_frame_array[block_index].allocated_order;
    int original_order = order;  // 保存原始 order 用於後續統計
    int merge_count = 0;         // 追踪合併次數
    int total_frames_merged = 0; // 追踪合併回收的總帧數
    char int_str[20]; // 用於轉換整數到字符串
    
    // 第一個打印語句
    // uart_send_string("Freeing block at index ");
    // utils_int_to_str(block_index, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" (address ");
    // uart_binary_to_hex(index2address(block_index));
    // uart_send_string(", order ");
    // utils_int_to_str(order, int_str);
    // uart_send_string(int_str);
    // uart_send_string(", size ");
    // utils_int_to_str(1 << order, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" frames, ");
    // utils_int_to_str((1 << order) * FRAME_SIZE, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" bytes)\r\n");
    
    buddy_frame_array[block_index].status = order; 
    int buddy_index = block_index ^ (1 << order);

    while (order < MAX_ORDER && 
           buddy_index < TOTAL_FRAME && 
           buddy_frame_array[buddy_index].status == order) {

        // 第二個打印語句
        // uart_send_string("Found buddy at index ");
        // utils_int_to_str(buddy_index, int_str);
        // uart_send_string(int_str);
        // uart_send_string(" (address ");
        // uart_binary_to_hex(index2address(buddy_index));
        // uart_send_string(", order ");
        // utils_int_to_str(order, int_str);
        // uart_send_string(int_str);
        // uart_send_string(") - merging blocks\r\n");
        
        merge_count++;
        
        if (free_frame_lists[order] == &buddy_frame_array[buddy_index]) {
            free_frame_lists[order] = buddy_frame_array[buddy_index].next;
            if (buddy_frame_array[buddy_index].next != BUDDY_NULL) {
                buddy_frame_array[buddy_index].next->prev = BUDDY_NULL;
            }
        } 
        else {
            if (buddy_frame_array[buddy_index].prev != BUDDY_NULL) {
                buddy_frame_array[buddy_index].prev->next = buddy_frame_array[buddy_index].next;
                if (buddy_frame_array[buddy_index].next != BUDDY_NULL) {
                    buddy_frame_array[buddy_index].next->prev = buddy_frame_array[buddy_index].prev;
                }
            }
        }

        if (buddy_index < block_index) {
            // 大索引被標記為 BELONGED_FRAME 並清除指針
            buddy_frame_array[block_index].status = BELONGED_FRAME;
            buddy_frame_array[block_index].allocated_order = -1;
            buddy_frame_array[block_index].next = BUDDY_NULL;
            buddy_frame_array[block_index].prev = BUDDY_NULL;
            
            // 第三個打印語句
            // uart_send_string("Block at index ");
            // utils_int_to_str(block_index, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" (address ");
            // uart_binary_to_hex(index2address(block_index));
            // uart_send_string(") merged into buddy at index ");
            // utils_int_to_str(buddy_index, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" (address ");
            // uart_binary_to_hex(index2address(buddy_index));
            // uart_send_string(")\r\n");
            
            // 更新 block_index 為較小的索引
            block_index = buddy_index;
        } 
        else {
            // 大索引被標記為 BELONGED_FRAME 並清除指針
            buddy_frame_array[buddy_index].status = BELONGED_FRAME;
            buddy_frame_array[buddy_index].allocated_order = -1;
            buddy_frame_array[buddy_index].next = BUDDY_NULL;
            buddy_frame_array[buddy_index].prev = BUDDY_NULL;
            
            // 第四個打印語句
            // uart_send_string("Buddy at index ");
            // utils_int_to_str(buddy_index, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" (address ");
            // uart_binary_to_hex(index2address(buddy_index));
            // uart_send_string(") merged into block at index ");
            // utils_int_to_str(block_index, int_str);
            // uart_send_string(int_str);
            // uart_send_string(" (address ");
            // uart_binary_to_hex(index2address(block_index));
            // uart_send_string(")\r\n");
        }

        order++;
        buddy_index = block_index ^ (1 << order);
    }

    buddy_frame_array[block_index].status = order;
    buddy_frame_array[block_index].allocated_order = -1;
    buddy_frame_array[block_index].next = free_frame_lists[order];
    buddy_frame_array[block_index].prev = BUDDY_NULL;
    
    if (free_frame_lists[order] != BUDDY_NULL) {
        free_frame_lists[order]->prev = &buddy_frame_array[block_index];
    }
    
    free_frame_lists[order] = &buddy_frame_array[block_index];
    
    // 計算並輸出合併的統計信息
    if (merge_count > 0) {
        total_frames_merged = (1 << order) - (1 << original_order);
        
        // 第五個打印語句
        // uart_send_string("Merge summary: ");
        // utils_int_to_str(merge_count, int_str);
        // uart_send_string(int_str);
        // uart_send_string(" buddies merged, block size increased from order ");
        // utils_int_to_str(original_order, int_str);
        // uart_send_string(int_str);
        // uart_send_string(" to order ");
        // utils_int_to_str(order, int_str);
        // uart_send_string(int_str);
        // uart_send_string("\r\n");
        
        // 第六個打印語句
        // uart_send_string("Total frames merged: ");
        // utils_int_to_str(total_frames_merged, int_str);
        // uart_send_string(int_str);
        // uart_send_string(", total memory saved: ");
        // utils_int_to_str(total_frames_merged * FRAME_SIZE, int_str);
        // uart_send_string(int_str);
        // uart_send_string(" bytes\r\n");
    } else {
        // 第七個打印語句
        // uart_send_string("No buddies found for merging\r\n");
    }
    
    // 第八個打印語句
    // uart_send_string("Block at index ");
    // utils_int_to_str(block_index, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" (address ");
    // uart_binary_to_hex(index2address(block_index));
    // uart_send_string(") freed and added to free list of order ");
    // utils_int_to_str(order, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" (size ");
    // utils_int_to_str(1 << order, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" frames, ");
    // utils_int_to_str((1 << order) * FRAME_SIZE, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" bytes)\r\n");
}

unsigned long index2address (unsigned int block_index) {
    return (unsigned long)(block_index * FRAME_SIZE);
}

unsigned int address2index(unsigned long address) {
    return (unsigned int)((address) / (unsigned long)FRAME_SIZE);
}

void slab_init() {
    for (int i = 0 ; i < SLAB_TYPE; i++){
        slab_start_list[i] = SLAB_NULL;
    }

    for(int i = 0; i < MAX_SLAB_FRAME_COUNT; i++){
        slab_frame_array[i].available = 1;
        slab_frame_array[i].slab_size = -1;
        slab_frame_array[i].start_address = -1;
        slab_frame_array[i].total_slabs = -1;
        slab_frame_array[i].free_slabs = -1;
        for (int j = 0; j < 32; j++) {  
            slab_frame_array[i].bitmap[j] = 0;  
        }
        slab_frame_array[i].next = SLAB_NULL;
        slab_frame_array[i].prev = SLAB_NULL;
    }
}

long slab_allocate(unsigned int size) {
    int slab_size = -1;
    int slab_type = -1;

    for (int i = 0; i < SLAB_TYPE; i++) {
        if (slab_sizes[i] >= size) {
            slab_size = slab_sizes[i];
            slab_type = i;
            break;
        }
    }

    if (slab_size == -1) {
        return -1; 
    }

    slab_t* slab_page = slab_start_list[slab_type];
    while (slab_page != SLAB_NULL) {
        if (slab_page->free_slabs > 0) {
            for (int i = 0; i < slab_page->total_slabs; i++) {
                int bitmap_index = i / 8; 
                int bit_pos = i % 8;  

                if ((slab_page->bitmap[bitmap_index] & (1UL << bit_pos)) == 0) {
                    slab_page->bitmap[bitmap_index] |= (1UL << bit_pos); 
                    slab_page->free_slabs--;
                    return (long)(slab_page->start_address + i * slab_size);
                }
            }
        }
        slab_page = slab_page->next;
    }

    int block_index = buddy_allocate(0);
    if (block_index == -1) {
        return -1; 
    }
    unsigned long address = index2address(block_index);

    slab_t* new_slab_frame = SLAB_NULL;
    for (int i = 0; i < MAX_SLAB_FRAME_COUNT; i++) {
        if (slab_frame_array[i].available == 1 && slab_frame_array[i].slab_size == -1) {
            new_slab_frame = &slab_frame_array[i];
            break;
        }
    }

    if (new_slab_frame == SLAB_NULL) {
        return -1; 
    }

    new_slab_frame->available = 0;
    new_slab_frame->slab_size = slab_size;
    new_slab_frame->start_address = address;
    new_slab_frame->total_slabs = FRAME_SIZE / slab_size;
    new_slab_frame->free_slabs = new_slab_frame->total_slabs;
    for (int i = 0; i < 32; i++) { 
        new_slab_frame->bitmap[i] = 0;
    }

    new_slab_frame->prev = SLAB_NULL;
    new_slab_frame->next = slab_start_list[slab_type];
    if (slab_start_list[slab_type] != SLAB_NULL) {
        slab_start_list[slab_type]->prev = new_slab_frame;
    }
    slab_start_list[slab_type] = new_slab_frame;

    new_slab_frame->bitmap[0] |= 0x01;  // 分配第一個 slab
    new_slab_frame->free_slabs--;  // 減少空閒 slab 數量

    return (long)(new_slab_frame->start_address);
}

void slab_free (unsigned long address, unsigned int size) {
    int slab_size = -1;
    int slab_type = -1;

    for (int i = 0; i < SLAB_TYPE; i++) {
        if (slab_sizes[i] >= size) {
            slab_size = slab_sizes[i];
            slab_type = i;
            break;
        }
    }

    if (slab_size == -1) {
        return ; 
    }

    slab_t* slab_page = SLAB_NULL;
    slab_page = slab_start_list[slab_type];
    while (slab_page != SLAB_NULL) {
        if (address >= slab_page->start_address &&
        address < slab_page->start_address + FRAME_SIZE) {
            
            int index = (address - slab_page->start_address) / slab_page->slab_size;
            int bitmap_index = index / 8;  
            int bit_pos = index % 8;       

            if ((slab_page->bitmap[bitmap_index] & (1UL << bit_pos)) != 0) {
                slab_page->bitmap[bitmap_index] &= ~(1UL << bit_pos);
                slab_page->free_slabs++; 

                if (slab_page->free_slabs == slab_page->total_slabs) {
                    buddy_free(address2index((unsigned long)slab_page->start_address));

                    slab_page->available = 1;
                    slab_page->slab_size = -1;
                    slab_page->start_address = -1;
                    slab_page->total_slabs = -1;
                    slab_page->free_slabs = -1;
                    for (int j = 0; j < 32; j++) {  
                        slab_page->bitmap[j] = 0;  
                    }
                    
                    if (slab_start_list[slab_type] == slab_page) {
                        slab_start_list[slab_type] = slab_page->next;
                    }

                    if (slab_page->next != SLAB_NULL) {
                        slab_page->next->prev = slab_page->prev;
                    }

                    if (slab_page->prev != SLAB_NULL) {
                        slab_page->prev->next = slab_page->next;
                    }

                    slab_page->next = SLAB_NULL;
                    slab_page->prev = SLAB_NULL;
                }
                return;
            }
        }
        slab_page = slab_page->next;
    }
}

void memory_reserve(unsigned long begin_address, unsigned long end_address) {
    int begin_block_index = address2index(begin_address);
    int end_block_index = address2index(end_address + FRAME_SIZE - 1);

    for (int i = begin_block_index; i < end_block_index; i++){
        if (i >= 0 && i < TOTAL_FRAME) {
            buddy_frame_array[i].status = RESERVED_FRAME;
        }
    }
}

void test_allocate_all_memory() {
    char int_str[20];
    uart_send_string("===== ALLOCATING ALL AVAILABLE MEMORY =====\r\n");
    
    g_allocated_count = 0;
    
    // 遍歷每個 order 的 free list
    for (int order = 0; order <= MAX_ORDER; order++) {
        uart_send_string("Allocating blocks of order ");
        utils_int_to_str(order, int_str);
        uart_send_string(int_str);
        uart_send_string("...\r\n");
        
        // 分配所有此 order 的 blocks
        while (free_frame_lists[order] != BUDDY_NULL) {
            int block_index = buddy_allocate(order);
            if (block_index == -1) break;  // 意外情況
            
            // 記錄已分配的 block
            if (g_allocated_count < MAX_ALLOCATED_BLOCKS) {
                g_allocated_indices[g_allocated_count] = block_index;
                g_allocated_orders[g_allocated_count] = order;
                g_allocated_count++;
            }
            
            // 寫入識別模式
            unsigned long addr = index2address(block_index);
            int frame_count = 1 << order;
            for (int i = 0; i < frame_count; i++) {
                // 在每個 frame 的開頭、中間和結尾寫入模式
                unsigned int* frame_start = (unsigned int*)(addr + i * FRAME_SIZE);
                unsigned int* frame_middle = (unsigned int*)(addr + i * FRAME_SIZE + FRAME_SIZE/2);
                unsigned int* frame_end = (unsigned int*)(addr + i * FRAME_SIZE + FRAME_SIZE - 4);
                
                *frame_start = g_test_pattern ^ block_index;  // 加入 block_index 以區分不同 blocks
                *frame_middle = g_test_pattern ^ (block_index + 1);
                *frame_end = g_test_pattern ^ (block_index + 2);
            }
        }
    }
    
    uart_send_string("Memory allocation complete. Allocated ");
    utils_int_to_str(g_allocated_count, int_str);
    uart_send_string(int_str);
    uart_send_string(" blocks.\r\n");
}

void memcheck() {
    char int_str[20];
    int errors = 0;
    
    uart_send_string("Checking memory integrity of ");
    utils_int_to_str(g_allocated_count, int_str);
    uart_send_string(int_str);
    uart_send_string(" allocated blocks...\r\n");
    
    for (int i = 0; i < g_allocated_count; i++) {
        unsigned int block_index = g_allocated_indices[i];
        int order = g_allocated_orders[i];
        unsigned long addr = index2address(block_index);
        int frame_count = 1 << order;
        
        for (int j = 0; j < frame_count; j++) {
            unsigned int* frame_start = (unsigned int*)(addr + j * FRAME_SIZE);
            unsigned int* frame_middle = (unsigned int*)(addr + j * FRAME_SIZE + FRAME_SIZE/2);
            unsigned int* frame_end = (unsigned int*)(addr + j * FRAME_SIZE + FRAME_SIZE - 4);
            
            unsigned int expected_start = g_test_pattern ^ block_index;
            unsigned int expected_middle = g_test_pattern ^ (block_index + 1);
            unsigned int expected_end = g_test_pattern ^ (block_index + 2);
            
            if (*frame_start != expected_start ||
                *frame_middle != expected_middle ||
                *frame_end != expected_end) {
                
                errors++;
                if (errors <= 5) {  // 只報告前 5 個錯誤
                    uart_send_string("Corruption detected in block at index ");
                    utils_int_to_str(block_index, int_str);
                    uart_send_string(int_str);
                    uart_send_string(", frame ");
                    utils_int_to_str(j, int_str);
                    uart_send_string(int_str);
                    uart_send_string("\r\n");
                }
            }
        }
    }
    
    if (errors == 0) {
        uart_send_string("Memory check passed! All allocated memory is intact.\r\n");
    } else {
        uart_send_string("Memory check failed with ");
        utils_int_to_str(errors, int_str);
        uart_send_string(int_str);
        uart_send_string(" corrupted frames.\r\n");
    }
}

void test_free_all_memory() {
    // char int_str[20];
    // uart_send_string("Freeing all ");
    // utils_int_to_str(g_allocated_count, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" allocated blocks...\r\n");
    
    for (int i = 0; i < g_allocated_count; i++) {
        buddy_free(g_allocated_indices[i]);
    }
    
    g_allocated_count = 0;
    uart_send_string("All memory freed successfully.\r\n");
}

void* simple_alloc(unsigned int size) {
    // 使用 unsigned long 而不是 unsigned int 來避免截斷 64 位指針
    if ((unsigned long)heap_head + size > (unsigned long)heap_limit) {
        uart_send_string("\r\nNot Enough Memory for simple_alloc");
        return (void*)0;
    }

    void* alloc_tail = heap_head;
    // 使用 unsigned long 來進行指針算術
    heap_head = (void*)((unsigned long)heap_head + size);

    return alloc_tail;
}

// 初始化 simple allocator
void init_simple_allocator() {
    heap_head = (void*)&_heap_start;
    heap_limit = (void*)&_heap_end;
    
    char int_str[20];
    // uart_send_string("\r\nHeap area: 0x");
    // uart_binary_to_hex((unsigned long)heap_head);
    // uart_send_string(" - 0x");
    // uart_binary_to_hex((unsigned long)heap_limit);
    // uart_send_string(" (size: ");
    // utils_int_to_str((unsigned long)heap_limit - (unsigned long)heap_head, int_str);
    // uart_send_string(int_str);
    // uart_send_string(" bytes)\r\n");
}

void mem_init() {
    init_simple_allocator();
    
    // 使用 simple allocator 分配內存
    buddy_frame_array = (buddy_t*)simple_alloc(sizeof(buddy_t) * TOTAL_FRAME);
    if (!buddy_frame_array) {
        uart_send_string("\r\nFailed to allocate buddy_frame_array");
        return;
    }
    
    free_frame_lists = (buddy_t**)simple_alloc(sizeof(buddy_t*) * (MAX_ORDER + 1));
    if (!free_frame_lists) {
        uart_send_string("\r\nFailed to allocate free_frame_lists");
        return;
    }
    
    slab_frame_array = (slab_t*)simple_alloc(sizeof(slab_t) * MAX_SLAB_FRAME_COUNT);
    if (!slab_frame_array) {
        uart_send_string("\r\nFailed to allocate slab_frame_array");
        return;
    }
    
    slab_start_list = (slab_t**)simple_alloc(sizeof(slab_t*) * SLAB_TYPE);
    if (!slab_start_list) {
        uart_send_string("\r\nFailed to allocate slab_start_list");
        return;
    }

    for (int i = 0; i < TOTAL_FRAME; i++) {
        buddy_frame_array[i].status = BELONGED_FRAME;
        buddy_frame_array[i].index = i;
        buddy_frame_array[i].next = BUDDY_NULL;
        buddy_frame_array[i].prev = BUDDY_NULL;
    }

    memory_reserve(0x0, 0x1000);

    unsigned long kernel_start_address = (unsigned long)&_kernel_start;
    unsigned long kernel_end_address = (unsigned long)&_kernel_end;
    memory_reserve(kernel_start_address, kernel_end_address);

    unsigned long cpio_start_address = (unsigned long)cpio_addr;
    unsigned long cpio_end_address = get_cpio_end_address();
    memory_reserve(cpio_start_address, cpio_end_address);

    unsigned long fdt_start_address = (unsigned long)(dtb_base);
    unsigned long fdt_end_address = get_fdt_end_address();
    memory_reserve(fdt_start_address, fdt_end_address);

    unsigned long heap_start_address = (unsigned long)&_heap_start;
    unsigned long heap_end_address = (unsigned long)&_heap_end;
    memory_reserve(heap_start_address, heap_end_address);

    buddy_init();
    slab_init();
}

void* malloc(unsigned int request_size) {
    if (request_size == 0) {
        return NULL;
    }
    
    if (request_size < 4096) {
        long addr = slab_allocate(request_size);
        if (addr == -1) {
            uart_send_string("Slab allocation failed\r\n");
            return NULL;
        }
        return (void*)addr;
    }
    
    int order = size2order(request_size);
    int block_index = buddy_allocate(order);
    if (block_index == -1) {
        uart_send_string("Buddy allocation failed\r\n");
        return NULL;
    }

    return (void*)index2address(block_index);
}

void free(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    unsigned long addr = (unsigned long)ptr;
    int found_in_slab = 0;
    for (int i = 0; i < SLAB_TYPE; i++) {
        slab_t* slab_page = slab_start_list[i];
        while (slab_page != SLAB_NULL) {
            if (addr >= slab_page->start_address && 
                addr < slab_page->start_address + FRAME_SIZE) {
                slab_free(addr, slab_sizes[i]);
                found_in_slab = 1;
                break;
            }
            slab_page = slab_page->next;
        }
        if (found_in_slab) break;
    }
    
    if (found_in_slab) {
        return;
    }
    
    unsigned int block_index = address2index(addr);
    
    if (block_index < TOTAL_FRAME && 
        buddy_frame_array[block_index].status == ALLOCATED_FRAME) {
        buddy_free(block_index);
        return;
    }
    
    uart_send_string("Error: Unable to free memory at address 0x");
    uart_binary_to_hex(addr);
    uart_send_string("\r\n");
}