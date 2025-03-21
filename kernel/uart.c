#include "header/uart.h"
#include "header/utils.h"

volatile char async_read_buf[UART_BUFFER_SIZE];
volatile char async_write_buf[UART_BUFFER_SIZE];
volatile unsigned int read_head = 0, read_tail = 0;
volatile unsigned int write_head = 0, write_tail = 0;

// sync uart

void uart_init() {
    register unsigned int r;
    //Since We've set alt5, we want to disable basic input/output
    //To achieve this, we need diable pull-up and pull-dwon
    *GPPUD = 0;   //  P101 top. 00- = off - disable pull-up/down 
    //Wait 150 cycles
    //this provides the required set-up time for the control signal 
    r=150; 
    while(r--) { 
        asm volatile("nop"); 
    }
    // GPIO control 54 pins
    // GPPUDCLK0 controls 0-31 pins
    // GPPUDCLK1 controls 32-53 pins
    // set 14,15 bits = 1 which means we will modify these two bits
    // trigger: set pins to 1 and wait for one clock
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; 
    while(r--) {
        asm volatile("nop"); 
    }
    *GPPUD = 0;           // remove control signal
    *GPPUDCLK0 = 0;        // flush GPIO setup


    r=500; while(r--) { asm volatile("nop"); }

    /* initialize UART */
    *AUX_ENABLES |=1;       
    //P.9: If set the mini UART is enabled. The UART will
    //immediately start receiving data, especially if the
    //UART1_RX line is low.
    //If clear the mini UART is disabled. That also disables
    //any mini UART register access 
    *AUX_MU_CNTL_REG = 0;
   //P.17 If this bit is set the mini UART receiver is enabled.
   //If this bit is clear the mini UART receiver is disabled
   //Prevent data exchange in initialization process
    *AUX_MU_IER_REG = 0;
   //Set AUX_MU_IER_REG to 0. 
   //Disable interrupt because currently you don’t need interrupt.
    *AUX_MU_LCR_REG = 3;       
   //P.14: 00 : the UART works in 7-bit mode
   //11(3) : the UART works in 8-bit mode
   //Cause 8 bits can use in ASCII, Unicode, Char
    *AUX_MU_MCR_REG = 0;
   //Don’t need auto flow control.
   //AUX_MU_MCR is for basic serial communication. Don't be too smart
    *AUX_MU_BAUD_REG = 270;
   //set BAUD rate to 115200(transmit speed)
   //so we need set AUX_MU_BAUD to 270 to meet the goal
    *AUX_MU_IIR_REG = 0xc6;
   // bit 6 bit 7 No FIFO. Sacrifice reliability(buffer) to get low latency    // 0xc6 = 11000110
   // Writing with bit 1 set will clear the receive FIFO
   // Writing with bit 2 set will clear the transmit FIFO
   // Both bits always read as 1 as the FIFOs are always enabled  
    /* map UART1 to GPIO pins */
    *AUX_MU_CNTL_REG = 3; // enable Transmitter,Receiver
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15 clear to 0
    r|=(2<<12)|(2<<15);    // set gpio14 and 15 to 010/010 which is alt5
    *GPFSEL1 = r;          // from here activate Trasmitter&Receiver
}

void uart_send_char(unsigned int c) {
    /* wait until we can send */
    while(!(*AUX_MU_LSR_REG&0x20)) {
        asm volatile("nop");
    }
    /* write the character to the buffer */
    *AUX_MU_IO_REG=c;
}

void uart_send_string(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if(str[i]=='\n')
            uart_send_char((char)'\r');
        uart_send_char((char)str[i]);
    }
}

char uart_recv_char() {
    /* wait until something is in the buffer */
    while(!(*AUX_MU_LSR_REG&0x01)) {
        asm volatile("nop");
    }
    /* read it and return */
    char r=(char)(*AUX_MU_IO_REG);
    /* convert carriage return to newline */
    return r=='\r'?'\n':r;
}

void uart_recv_string(char *buf, int buf_size) {
    for (int i = 0; i < buf_size; i ++) {
        buf[i] = uart_recv_char();
        if (buf[i] == '\0') {
            break;
        }
    }
    buf[buf_size - 1] = '\0';
}

char uart_recv_raw() {
    // Wait until data is available in the receive buffer
    while (!(*AUX_MU_LSR_REG & 0x01)) {
        asm volatile("nop");
    }
    // Read character from UART
    char r = (char)(*AUX_MU_IO_REG);
    return r;
}


void uart_binary_to_hex(unsigned int value) {
    char hex_str[11];  // "0x" (2) + 8 hex digits + '\0' (1) = 11
    const char hex_digits[] = "0123456789ABCDEF";

    // Add the "0x" prefix
    hex_str[0] = '0';
    hex_str[1] = 'x';

    // Convert the unsigned int to hexadecimal and store it in the string
    for (int i = 0; i < 8; i++) {
        hex_str[2 + i] = hex_digits[(value >> ((7 - i) * 4)) & 0xF]; // Corrected index and shift
    }

    hex_str[10] = '\0';  // Null-terminate the string

    // Send the string via UART
    uart_send_string(hex_str);
}

//  async uart
void init_async_buffer() {
    for(int i = 0 ; i < UART_BUFFER_SIZE; i++){
        async_read_buf[i] = 0;
        async_write_buf[i] = 0;
    }
}

void enable_uart_read_interrupt(){
    *AUX_MU_IER_REG |= 0x01;
}

void disable_uart_read_interrupt(){
    *AUX_MU_IER_REG &= (~0x01);
}

void enable_uart_write_interrupt(){
    *AUX_MU_IER_REG |= 0x02;
}

void disable_uart_write_interrupt(){
    *AUX_MU_IER_REG &= (~0x02);
}

void enable_uart_interrupt(){
    *ENABLE_IRQS_1 |= (1 << 29);
}

void disable_uart_interrupt(){
    *DISABLE_IRQS_1 |= (1 << 29);
}

void rx_interrupt_handler() {
    disable_interrupt();
    char ch = *((char*)AUX_MU_IO_REG); 
    async_read_buf[read_tail] = ch;
    read_tail = (read_tail + 1) % UART_BUFFER_SIZE; 
    enable_interrupt();
}

void tx_interrupt_handler() {
    while (write_head != write_tail) {
        while (!(*AUX_MU_LSR_REG & 0x20)) { // wait for UART tx is available
            asm volatile("nop");
        }
        disable_interrupt();
        char ch = async_write_buf[write_head];
        write_head = (write_head + 1) % UART_BUFFER_SIZE; 
        *AUX_MU_IO_REG = ch;
        enable_interrupt();
    }
}

void async_uart_handler_c() {
    disable_uart_interrupt();
    if (*AUX_MU_IIR_REG & 0x04){
        disable_uart_read_interrupt();
        rx_interrupt_handler();
    }
    else if (*AUX_MU_IIR_REG & 0x02){
        disable_uart_write_interrupt();
        tx_interrupt_handler();
    }
    enable_uart_interrupt();
}

unsigned int async_uart_gets(char* buffer, unsigned int len) {
    unsigned int read_len = 0;
    for (; read_len < len;) {
        while (read_head == read_tail) { // wait for input data
            enable_uart_read_interrupt();
        }
        
        disable_interrupt();
        buffer[read_len++] = async_read_buf[read_head]; 
        read_head = (read_head + 1) % UART_BUFFER_SIZE; 
        enable_interrupt();

        if (buffer[read_len - 1] == '\n' || buffer[read_len - 1] == '\r') { 
            buffer[read_len] = '\0';
            break;
        }
    }
    return read_len;
}

void async_uart_puts(char* str) {
    disable_interrupt();

    while (*str != '\0') {
        if (*str == '\n') {
            // Ensure '\n' is sent correctly (CRLF handling)
            async_write_buf[write_tail] = '\r';
            write_tail = (write_tail + 1) % UART_BUFFER_SIZE;
        }

        async_write_buf[write_tail] = *str;
        write_tail = (write_tail + 1) % UART_BUFFER_SIZE;
        
        str++;
    }

    enable_interrupt();
    enable_uart_write_interrupt();
}

void test_async_uart() {
    // Clear any pending input in the buffer first
    disable_interrupt();
    read_head = read_tail;  // Reset buffer pointers to clear any pending input
    enable_interrupt();
    
    // Now proceed with the test
    char buffer[UART_BUFFER_SIZE];

    unsigned int rlen = async_uart_gets(buffer, UART_BUFFER_SIZE);
    uart_send_string("\nGet ");
    uart_binary_to_hex(rlen);
    uart_send_string(" characters");
    uart_send_string("\r\n");

    async_uart_puts(buffer);
    int timeout = 100000;
    while(write_head != write_tail && timeout-- > 0) {
        for (int i = 0; i < 100; i++) {
            asm volatile("nop");
        }
    }
    uart_send_string("\n");
}
