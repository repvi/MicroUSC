// ESP32 Assembly Code

// In Xtensa architecture, a0 is reserved for the return address.
// a1 is used as the stack pointer.
// MOV is used for normal varaibles, and ADDI is used for immediate values.
// anything with i means immediate value. Immediate values are constant 16 bits long.

// l32r a2, my_constant  # Load the 32-bit constant at address 'my_constant' into register a2

.section .text
.global s_atomic_add
.align 4

s_atomic_add:
    retry:
        //entry a1, 16          // Create stack frame
        l32i a4, a2, 0   // Load the value at address a2 into a4. a dereference of a pointer.
        addi a5, a4, a3  // Add the immediate value a3 to a4 (uint32_t value)
        //l32i a5, a2, 4   // Load the value at address a2 + 4 into a5
        s32c1i a5, a2, 0  // Store the value in a5 at address a2 and return the old value in a4
        bnez a5, retry        // Retry on failure
    ret   // Return without setting any value