#ifndef BUDDY_H
#define BUDDY_H

#include "cpio.h"
#include "fdt.h"
#include "uart.h"
#include "utils.h"

#define FRAME_SIZE 4096
#define MEM_END 0x3C000000 // 24576 total frames in system
// #define MEM_END 0x00040000
#define TOTAL_FRAME MEM_END / FRAME_SIZE
#define BELONGED_FRAME -1
#define ALLOCATED_FRAME -2 // by buddy system 
#define RESERVED_FRAME -3 // pre-allocated by kernel

#define MAX_ORDER 20
#define BUDDY_NULL ((buddy_t*)0)
#define SLAB_NULL ((slab_t*)0)

typedef struct buddy_t {
    int status;          // 與原始 frame_array 相同的狀態定義
    unsigned int index;  // 此 buddy 的 frame 索引
    struct buddy_t* next;  // 指向下一個同大小的 free buddy
    struct buddy_t* prev;  // 指向上一個同大小的 free buddy
    int allocated_order;
} buddy_t;

// 改為指針變量，用於動態分配
extern buddy_t* buddy_frame_array;  // 指向動態分配的內存幀描述符數組
extern buddy_t** free_frame_lists;  // 指向每個 order 的 free list 頭

typedef struct slab_t {
    int available ; // is this slab page frame descriptor available
    int slab_size;  // how large is the slab this page frame stores  
    unsigned long start_address;   // the page frame address  
    int total_slabs;     //  total slabs can store inside this page frame, given the slab_size
    int free_slabs;  // total free slab slot that can use in this page frame
    unsigned char bitmap[32]; // bitmap stores the information of whether each slab slot is used in this page frame
    struct slab_t* next; // points to next page frame descriptor thats stores the same slab size objects
    struct slab_t* prev;
} slab_t;

#define SLAB_TYPE 10
#define MAX_SLAB_FRAME_COUNT (TOTAL_FRAME / 4)
extern unsigned int slab_sizes[];
extern slab_t* slab_frame_array;    // 改為指針，用於動態分配
extern slab_t** slab_start_list;    // 改為指針，用於動態分配

// 堆區管理
extern void* heap_head;
extern void* heap_limit;
extern char _heap_start;
extern char _heap_end;

// 函數聲明
void* simple_alloc(unsigned int size);
void init_simple_allocator();

int size2order(unsigned int);
void buddy_init();
int buddy_allocate(int);
void buddy_free(int);
unsigned long index2address(unsigned int);
unsigned int address2index(unsigned long);
void slab_init();
long slab_allocate(unsigned int);
void slab_free(unsigned long, unsigned int);

void memory_reserve(unsigned long, unsigned long);
void mem_init();

#define MAX_ALLOCATED_BLOCKS 30000
extern unsigned int g_allocated_indices[MAX_ALLOCATED_BLOCKS];
extern int g_allocated_orders[MAX_ALLOCATED_BLOCKS];
extern int g_allocated_count;
extern int g_test_pattern;
void test_allocate_all_memory();
void memcheck();
void test_free_all_memory();

void* malloc(unsigned int);
void free(void*);

#endif // BUDDY_H
