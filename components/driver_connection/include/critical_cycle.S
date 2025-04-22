// ESP32 Assembly Code

// In Xtensa architecture, a0 is reserved for the return address.
// a1 is used as the stack pointer.
// MOV is used for normal varaibles, and ADDI is used for immediate values.
// anything with i means immediate value. Immediate values are constant 16 bits long.

// l32r a2, my_constant  # Load the 32-bit constant at address 'my_constant' into register a2

//l32i a5, a2, 4   // Load the value at address a2 + 4 into a5

// retw is used to deallocate memroy and return to the caller.
// wsr is used to write to the special register scompare1, which is used for atomic operations.
.section .iram1,"ax"
.global s_atomic_set
.align 4

s_atomic_set:
    entry sp, 16          // Create stack frame
    retry:
        wsr a4, scompare1 // Set the compare register to a4
        l32i    a4, a2, 0  // Load current value from [a2]
        add     a5, a4, a3 // Compute new value (a5 = a4 + a3)
        s32c1i  a5, a2, 0  // Atomic compare-and-swap
        bnez    a5, retry   // Retry on failure
    mov     a2, a4         // Return original value
    //ret                    // Exit
    retw           // Deallocate memory and return to caller



.section .text
.global s_atomic_get
.align 4

s_atomic_get:
    //entry a1, 16          // Create stack frame
    l32i    a4, a2, 0     // Load current value from [a2]
    ret                    // Exit