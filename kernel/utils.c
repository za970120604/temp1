#include "header/utils.h"

int utils_string_compare(char* str1, char* str2) {
    while (*str1 || *str2) {
        // Skip '\r', '\n', '\0' in str1
        while (*str1 == '\r' || *str1 == '\n' || *str1 == '\0') str1++;
        // Skip '\r', '\n', '\0' in str2
        while (*str2 == '\r' || *str2 == '\n' || *str2 == '\0') str2++;

        // If both reached the end, strings are considered equal
        if (!*str1 && !*str2) return 1;
        
        // Compare the characters, if different return 0
        if (*str1 != *str2) return 0;

        str1++;
        str2++;
    }
    return 1;
}

unsigned long utils_HexStr2Int(char *s, int char_size) {
    unsigned long num = 0;
    for (int i = 0; i < char_size; i++) {
        num = num * 16;
        if (*s >= '0' && *s <= '9') {
            num += (*s - '0');
        } else if (*s >= 'A' && *s <= 'F') {
            num += (*s - 'A' + 10);
        } else if (*s >= 'a' && *s <= 'f') {
            num += (*s - 'a' + 10);
        }
        s++;
    }
    return num;
}

unsigned long utils_DecStr2Int(char *s, int char_size) {
    unsigned long num = 0;
    for (int i = 0; i < char_size; i++) {
        // Ensure that the character is a valid decimal digit ('0' - '9')
        if (*s >= '0' && *s <= '9') {
            num = num * 10 + (*s - '0');  // Multiply by 10 (shift left) and add the current digit
        } 
        else {
            return 0;  // Return 0 for invalid character (could also handle errors differently)
        }
        s++;
    }
    return num;
}

unsigned long utils_align(unsigned long size, unsigned long s) {
    if (s == 0 || (s & (s - 1)) != 0) return size; // 確保 s 是 2 的次方
    unsigned long mask = s - 1;
    return (size + mask) & ~mask;
}

unsigned long utils_strlen(char *s) {
    unsigned long i = 0;
	while (s[i]) i++;
	return i;
}

unsigned int utils_change_endian_32(unsigned int val){
    return ((val >> 24) & 0x000000FF) | 
            ((val >> 8)  & 0x0000FF00) | 
            ((val << 8)  & 0x00FF0000) | 
            ((val << 24) & 0xFF000000);
}

void utils_int_to_str(int value, char* str) {
    int i = 0;
    int is_negative = 0;
    
    // 處理 0 的特殊情況
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    // 處理負數
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    // 轉換數字到字符串 (反向)
    while (value != 0) {
        int digit = value % 10;
        str[i++] = digit + '0';
        value /= 10;
    }
    
    // 添加負號 (如果需要)
    if (is_negative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    
    // 反轉字符串
    int len = i;
    for (i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

void utils_memcpy(char* src, char* dst, unsigned int len){
    for (int i = 0; i < len; i++){
        dst[i] = src[i];
    }
}
