#ifndef CPIO_H
#define CPIO_H

extern void* cpio_addr;  // Declaration only
void cpio_ls();
void cpio_cat(char* );
char* find_file(char *);
void cpio_exec(char *);
unsigned long get_cpio_end_address();
unsigned int get_cpio_file_size(char*);

struct cpio_header {
    /*  In https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
        New ASCII Format
            The "new" ASCII format uses 8-byte hexadecimal fields for all numbers*/

    char c_magic[6];    // The integer value 070707
    char c_ino[8];      // inode numbers from the disk
    char c_mode[8];     // The mode	specifies both the regular permissions and the file type
    char c_uid[8];      // The numeric user	id and group id	of the owner.
    char c_gid[8];      
    char c_nlink[8];    // The number of links to this file. Directories always have a value of	at least two here
    char c_mtime[8];    // Modification time of the	file
    char c_filesize[8]; // The size	of the file. Likewise, the file data is padded to a multiple of four bytes
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8]; // The number of bytes in the pathname that	follows the header. The pathname is followed by NUL bytes so that the total size of the fixed header plus pathname is a multiple of four.
    char c_check[8];    // This field is always set	to zero	by writers and ignored by readers.
};
#endif // CPIO_H