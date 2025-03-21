#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#define MAX_SIMPLE_MALLOC_SIZE 16384
void* task_malloc(unsigned int);
void* user_malloc(unsigned int );

#endif // ALLOCATOR_H
