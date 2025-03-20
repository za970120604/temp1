#ifndef UTILS_H
#define UTILS_H

int utils_string_compare(char*, char*);
unsigned long utils_HexStr2Int(char *, int );
unsigned long utils_DecStr2Int(char *, int );
unsigned long utils_align(unsigned long , unsigned long );
unsigned long utils_strlen(char *);
unsigned int utils_change_endian_32(unsigned int);
void utils_int_to_str(int , char* ); 
#define NULL (void*)0

#endif // UTILS_H