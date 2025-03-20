#include "header/fdt.h"
#include "header/utils.h"
#include "header/uart.h"
void* dtb_base = 0;

unsigned int initramfs_callback(char *node_name, char *prop_name, void *prop_value, int prop_len, char* target_node_name, char* target_prop_name) {
    if (utils_string_compare(node_name, target_node_name) > 0 && utils_string_compare(prop_name, target_prop_name) > 0) {
        unsigned int initrd_start = utils_change_endian_32(*(unsigned int *)prop_value);
        uart_send_string("Target Node: ");
        uart_send_string(target_node_name);
        uart_send_string(", Target device: ");
        uart_send_string(target_prop_name);
        uart_send_string(", starts at ");
        uart_binary_to_hex(initrd_start);
        uart_send_string("\r\n");
        return initrd_start;
    }
    return 0;
}

void* fdt_traverse(fdt_callback_t callback, char* target_node_name, char* target_prop_name) {
    asm volatile("mov %0, x20" :  "=r"(dtb_base));
    struct fdt_header *header = (struct fdt_header *)dtb_base;
    if (utils_change_endian_32(header->magic) != FDT_MAGIC_NUMBER) {
        uart_send_string("Invalid FDT magic number!\n");
        return NULL;
    } 
    else {
        uart_send_string("Valid FDT magic number!\n");
    }

    /*
        The structure block is composed of a sequence of pieces, each beginning with a token, that is, a big-endian 32-bit integer. 
        Some tokens are followed by extra data, the format of which is determined by the token value. 
        All tokens shall be aligned on a 32-bit boundary, which may require padding bytes (with a value of 0x0) to be inserted after the previous token’s data.
    */
    void* target_address = NULL;
    unsigned int *dt_struct = (unsigned int *)((char *)dtb_base + utils_change_endian_32(header->off_dt_struct));
    char *dt_strings = (char *)dtb_base + utils_change_endian_32(header->off_dt_strings);
    char *current_node = NULL;

    while ((unsigned long)dt_struct < (unsigned long)dtb_base + utils_change_endian_32(header->totalsize)) {
        unsigned int token = utils_change_endian_32(*dt_struct++);

        if (token == FDT_BEGIN_NODE) {  
            current_node = (char *)dt_struct;
            /*
                The FDT_BEGIN_NODE token marks the beginning of a node’s representation. 
                It shall be followed by the node’s unit name as extra data. 
                The name is stored as a null-terminated string, and shall include the unit address (see section 2.2.1), if any. 
                The node name is followed by zeroed padding bytes, if necessary for alignment.
            */
            dt_struct = (unsigned int *)utils_align((unsigned long)dt_struct + utils_strlen(current_node) + 1, 4);
        } 
        else if (token == FDT_PROP) {
            /*
                The FDT_PROP token marks the beginning of the representation of one property in the devicetree. 
                It shall be followed by extra data describing the property. 
                This data consists first of the property’s length and name represented as the following C structure:
                    struct {
                        uint32_t len;
                        uint32_t nameoff;
                    }
                Both the fields in this structure are 32-bit big-endian integers.
                len gives the length of the property’s value in bytes (which may be zero, indicating an empty property, see section 2.2.4.2).
                nameoff gives an offset into the strings block (see section 5.5) at which the property’s name is stored as a null-terminated string.
                After this structure, the property’s value is given as a byte string of length len. 
                This value is followed by zeroed padding bytes (if necessary) to align to the next 32-bit boundary and then the next token.
            */
            unsigned int prop_len = utils_change_endian_32(*dt_struct++);
            unsigned int name_offset = utils_change_endian_32(*dt_struct++);
            char *prop_name = dt_strings + name_offset;
            void *prop_value = dt_struct;
            dt_struct = (unsigned int *)utils_align((unsigned long)dt_struct + prop_len, 4);
            
            if (callback) {
                unsigned int address = callback(current_node, prop_name, prop_value, prop_len, target_node_name, target_prop_name);
                if (address != 0) {
                    target_address = (void*)(unsigned long)address;
                }
            }
        } 
        else if (token == FDT_END_NODE) {  
            continue;
        } 
        else if (token == FDT_NOP) {  
            continue;
        } 
        else if (token == FDT_END) {  
            break;
        }
    }
    return target_address;
}

unsigned long get_fdt_end_address() {    
    struct fdt_header *header = (struct fdt_header *)dtb_base;
    unsigned int fdt_size = utils_change_endian_32(header->totalsize);
    return (unsigned long)dtb_base + fdt_size;
}