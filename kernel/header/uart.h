#ifndef UART_H
#define UART_H

#include "gpio.h"  // Ensures MMIO_BASE is defined
#include "exception.h" // disable/enable interrupt

/*
P8.

2 
Auxiliaries: UART1 & SPI1, SPI2 
2.1 Overview 
The Device has three Auxiliary peripherals: One mini UART and two SPI masters. These 
three peripheral are grouped together as they share the same area in the peripheral register 
map and they share a common interrupt. Also all three are controlled by the auxiliary enable 
register. 

*/

#define AUX_IRQ                 ((volatile unsigned int*)(MMIO_BASE+0x00215000))
#define AUX_ENABLES             ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO_REG           ((volatile unsigned int*)(MMIO_BASE+0x00215040))
#define AUX_MU_IER_REG          ((volatile unsigned int*)(MMIO_BASE+0x00215044))
#define AUX_MU_IIR_REG          ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR_REG          ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR_REG          ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR_REG          ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR_REG          ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH          ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL_REG         ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT_REG         ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD_REG         ((volatile unsigned int*)(MMIO_BASE+0x00215068))
#define AUX_SPI0_CNTL0_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215080))
#define AUX_SPI0_CNTL1_REG      ((volatile unsigned int*)(MMIO_BASE+0x00215084))
#define AUX_SPI0_STAT_REG       ((volatile unsigned int*)(MMIO_BASE+0x00215088))
#define AUX_SPI0_IO_REG         ((volatile unsigned int*)(MMIO_BASE+0x00215090))
#define AUX_SPI0_PEEK_REG       ((volatile unsigned int*)(MMIO_BASE+0x00215094))
#define AUX_SPI1_CNTL0_REG      ((volatile unsigned int*)(MMIO_BASE+0x002150C0))
#define AUX_SPI1_CNTL1_REG      ((volatile unsigned int*)(MMIO_BASE+0x002150C4))
#define AUX_SPI1_STAT_REG       ((volatile unsigned int*)(MMIO_BASE+0x002150C8))
#define AUX_SPI1_IO_REG         ((volatile unsigned int*)(MMIO_BASE+0x002150D0))
#define AUX_SPI1_PEEK_REG       ((volatile unsigned int*)(MMIO_BASE+0x002150D4))

#define IRQ_PENDING_1           ((volatile unsigned int*)(MMIO_BASE+0x0000b204))
#define ENABLE_IRQS_1		    ((volatile unsigned int*)(MMIO_BASE+0x0000b210))
#define DISABLE_IRQS_1          ((volatile unsigned int*)(MMIO_BASE+0x0000b21c))
#define UART_BUFFER_SIZE 2048

// utils
void uart_init();
void uart_send_char(unsigned int);
void uart_send_string(char *);
char uart_recv_char();
void uart_recv_string(char *, int);
void uart_binary_to_hex(unsigned int);
char uart_recv_raw();

// async buffer
void init_async_buffer();

// uart io interrupt control
void enable_uart_read_interrupt();
void disable_uart_read_interrupt();

void enable_uart_write_interrupt();
void disable_uart_write_interrupt();

// interrupt control
void enable_uart_interrupt(); 
void disable_uart_interrupt();

// uart tx/rx interrupt handler
void rx_interrupt_handler();
void tx_interrupt_handler();
void async_uart_handler_c();

// async utils
unsigned int async_uart_gets(char *, unsigned int );
void async_uart_puts(char *);
void test_async_uart();

#endif  // UART_H
