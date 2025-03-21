.section ".text.boot"
.global _kernel_boot
.global _exception_vector_table
.global _enable_core_timer
.global _disable_core_timer
.global _child_fork_ret
.type _child_fork_ret, @function

_kernel_boot:
    mrs     x1, mpidr_el1 // mpidr_el1: Multiprocessor Affinity Register
    and     x1, x1, #3   // get cpu id and place it in x1 register
    cbz     x1, _el_vt_set // if cpu id is zero (main core), go to "_el_vt_set", otherwise go to "_halt"

_halt:  
    wfe
    b       _halt // wait for event, halt

_el_vt_set:
    mrs     x1, CurrentEL
    ubfx    x1, x1, #2, #2
    cmp     x1, #2
    bne     _halt

    bl      _from_el2_to_el1
    mrs     x1, CurrentEL
    ubfx    x1, x1, #2, #2
    cmp     x1, #1
    bne     _halt
    
    bl      _set_exception_vector_table

_set: 
    // The stack grows downward in AArch64 (from high (0x80000) to low memory (0x79999, 0x79998) addresses).
    // We need to ensure that sp is set to a valid, writable memory region before using the stack.
    // It must be aligned properly according to AAPCS64 (Arm Procedure Call Standard), meaning sp should be 16-byte aligned.
    // since 0x80000 is 16-byte aligned, we can use it as the stack pointer.
    ldr     x1, =_kernel_boot  // load address of _kernel_boot to x1
    mov     sp, x1 // stack pointer before _kernel_boot
    
    ldr     x1, =_bss_section_start
    ldr     w2, =_bss_size

_clear_bss:
    cmp     w2, #8
    blt     _clear_remaining   // If w2 < 8, handle the remaining bytes
    str     xzr, [x1], #8      // Clear 8 bytes
    sub     w2, w2, #8         // Decrease size
    b       _clear_bss         // Continue loop

_clear_remaining:
    cbz     w2, _kernel_main_entry           // If nothing left, jump to main
    strb    wzr, [x1], #1      // Clear remaining 1 byte at a time
    sub     w2, w2, #1
    cbnz    w2, _clear_remaining  // Loop until w2 = 0

_kernel_main_entry:  
    bl      main
    b      _halt

_from_el2_to_el1:
    mov     x0, #(1 << 31) // EL1 uses aarch64
    msr     hcr_el2, x0
    mov     x0, #0x345 // EL1h (SPSel = 1) with interrupt disabled
    msr     spsr_el2, x0
    msr     elr_el2, lr
    eret // return to EL1

.macro _save_all
    sub     sp, sp, 32 * 10
    stp     x0, x1, [sp ,16 * 0]
    stp     x2, x3, [sp ,16 * 1]
    stp     x4, x5, [sp ,16 * 2]
    stp     x6, x7, [sp ,16 * 3]
    stp     x8, x9, [sp ,16 * 4]
    stp     x10, x11, [sp ,16 * 5]
    stp     x12, x13, [sp ,16 * 6]
    stp     x14, x15, [sp ,16 * 7]
    stp     x16, x17, [sp ,16 * 8]
    stp     x18, x19, [sp ,16 * 9]
    stp     x20, x21, [sp ,16 * 10]
    stp     x22, x23, [sp ,16 * 11]
    stp     x24, x25, [sp ,16 * 12]
    stp     x26, x27, [sp ,16 * 13]
    stp     x28, x29, [sp ,16 * 14]
    str     x30, [sp, 16 * 15]

    mrs     x0,  spsr_el1
    str     x0,  [sp, 16 * 15 + 8]
    mrs     x0,  elr_el1
    str     x0,  [sp, 16 * 16]
    mrs     x0,  sp_el0
    str     x0,  [sp, 16 * 16 + 8]

    mov     x0, sp    

.endm

.macro _load_all
    ldp     x0, x1, [sp ,16 * 0]
    ldp     x2, x3, [sp ,16 * 1]
    ldp     x4, x5, [sp ,16 * 2]
    ldp     x6, x7, [sp ,16 * 3]
    ldp     x8, x9, [sp ,16 * 4]
    ldp     x10, x11, [sp ,16 * 5]
    ldp     x12, x13, [sp ,16 * 6]
    ldp     x14, x15, [sp ,16 * 7]
    ldp     x16, x17, [sp ,16 * 8]
    ldp     x18, x19, [sp ,16 * 9]
    ldp     x20, x21, [sp ,16 * 10]
    ldp     x22, x23, [sp ,16 * 11]
    ldp     x24, x25, [sp ,16 * 12]
    ldp     x26, x27, [sp ,16 * 13]
    ldp     x28, x29, [sp ,16 * 14]
    ldr     x30, [sp, 16 * 15]
    
    ldr     x0,  [sp, 16 * 15 + 8]
    msr     spsr_el1, x0
    ldr     x0,  [sp, 16 * 16]
    msr     elr_el1, x0
    ldr     x0,  [sp, 16 * 16 + 8]
    msr     sp_el0, x0

    ldp     x0, x1, [sp ,16 * 0] 
    
    add     sp, sp, 32 * 10
.endm

_set_exception_vector_table:
    adr     x0, _exception_vector_table
    msr     vbar_el1, x0
    ret

_naive_irq_handler:
    _save_all
    bl      naive_irq_entry
    _load_all
    eret

_el1_irq_handler:
    _save_all
    bl      el1_irq_entry
    _load_all
    eret

_el0_irq_handler:
    _save_all
    bl      el0_irq_entry
    _load_all
    eret


_el0_sync_handler:
    _save_all
    bl      el0_sync_entry
    _load_all
    eret

_enable_core_timer:
    // Enable the core timer
    mov     x0, 1                  // Set x0 to 1
    msr     cntp_ctl_el0, x0       // Write to cntp_ctl_el0 to enable the core timer

    // Set the expired time
    // mrs     x0, cntfrq_el0         // Read the value of cntfrq_el0 (frequency)
    // msr     cntp_tval_el0, x0      // Write this value to cntp_tval_el0 to set the timer

    // Unmask the timer interrupt (0x40000040 is the address of the interrupt control register)
    mov     x0, 2                  // Set x0 to 2 (unmask the interrupt)
    ldr     x1, =0x40000040        // Load the address 0x40000040 into x1 (GIC interrupt control register)
    str     w0, [x1]               // Store the value in w0 (2) to the memory address in x1

    mrs     x0, cntfrq_el0
    lsr     x0, x0, #5
    msr     cntp_tval_el0, x0
    ret                            // Return from the function

_disable_core_timer:
    // Disable the core timer
    mov     x0, 0                  // Set x0 to 0
    msr     cntp_ctl_el0, x0       // Write to cntp_ctl_el0 to disable the core timer

    // Set the expired time
    mrs     x0, cntfrq_el0         // Read the value of cntfrq_el0 (frequency)
    msr     cntp_tval_el0, x0      // Write this value to cntp_tval_el0 to set the timer

    // Mask the timer interrupt (0x40000040 is the address of the interrupt control register)
    mov     x0, 0                  // Set x0 to 0 (mask the interrupt)
    ldr     x1, =0x40000040        // Load the address 0x40000040 into x1 (GIC interrupt control register)
    str     w0, [x1]               // Store the value in w0 (0) to the memory address in x1
    ret                            // Return from the function

.align 11
_exception_vector_table:
    b _naive_irq_handler        // Synchronous EL1t
    .align 7 
    b _naive_irq_handler        // IRQ EL1t
    .align 7 
    b _naive_irq_handler        // FIQ EL1t
    .align 7 
    b _naive_irq_handler        // SError EL1t
    .align 7 

    b _naive_irq_handler        // Synchronous EL1h
    .align 7
    b _el1_irq_handler          // IRQ EL1h
    .align 7 
    b _naive_irq_handler        // FIQ EL1h
    .align 7 
    b _naive_irq_handler        // SError EL1h
    .align 7 

    b _naive_irq_handler        // Synchronous EL0_64  
    .align 7 
    b _el0_irq_handler          // IRQ EL0_64
    .align 7 
    b _naive_irq_handler        // FIQ EL0_64
    .align 7 
    b _naive_irq_handler        // SError EL0_64
    .align 7 

    b _naive_irq_handler        // Synchronous EL0_32
    .align 7 
    b _naive_irq_handler        // IRQ EL0_32
    .align 7 
    b _naive_irq_handler        // FIQ EL0_32
    .align 7 
    b _naive_irq_handler        // SError EL0_32
    .align 7

_child_fork_ret:
    bl _enable_core_timer
    bl enable_interrupt
    _load_all
    eret
