// ESP32 Assembly Code

// In Xtensa architecture, a0 is reserved for the return address.
// a1 is used as the stack pointer.
// MOV is used for normal varaibles, and ADDI is used for immediate values.
// anything with i means immediate value. Immediate values are constant 16 bits long.

// l32r a2, my_constant  # Load the 32-bit constant at address 'my_constant' into register a2

//l32i a5, a2, 4   // Load the value at address a2 + 4 into a5

// retw is used to deallocate memroy and return to the caller.
// wsr is used to write to the special register scompare1, which is used for atomic operations.
.section .data
.align 4
lock: .word 0x0, 0x0, 0x0, 0x0, 0x0  // Initialize lock variable to 0

.section .iram1,"ax"
.align 4

.global get_atomic_lock
.type get_atomic_lock, @function
get_atomic_lock:
    entry   sp, 16           // Create stack frame
    mov     a8, 0x0          // Initialize a8 to 0
    1:
        sub     a9, a8, 10
        add     a8, a8, 1       // increment a8 by 1, (a8++)
        bltz    a9, 1b          // Loop until a8 is less than 10
    
        rsil    a9, 15          // Disable interrupts, reusing register a9
        movi    a10, lock,      // Load the address of the lock variable into a2
        slli    a11, a2, 2      // Shift left logical immediate by 2 bits (multiply by 4)    
        addi    a10, a10, a11

        
        mov     a8, 0x0         // Initialize a8 to 0

    .size get_atomic_lock, .-get_atomic_lock
    .align 4

.global s_atomic_set
.type s_atomic_set, @function
s_atomic_set:
    entry   sp, 16         // Create stack frame
    retry:
        rsil    a8, 15     // Disable interrupts
        l32i    a9, a2, 0  // Load current value from [a2]
        add     a10, a9, a3 // Compute new value (a5 = a10 + a3)
        s32c1i  a10, a2, 0  // Atomic compare-and-swap
        bnez    a5, retry  // Retry on failure
    
    mov     a2, a4         // Return original value
    retw           // Deallocate memory and return to caller
    .size s_atomic_set, .-s_atomic_set
    .align 4

.global s_atomic_get
.type s_atomic_get, @function
s_atomic_get:
    l32i    a4, a2, 0     // Load current value from [a2]
    ret
    .size s_atomic_get, .-s_atomic_get
    .align 4