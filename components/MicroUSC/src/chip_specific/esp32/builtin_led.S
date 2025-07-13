#include "soc/gpio_reg.h"

#define BUILTIN_LED   2

.section .text
.align 4

.Lgpio_enable:      .word GPIO_ENABLE_REG
.Lgpio_out:         .word GPIO_OUT_REG
.Lgpio_func_cfg:    .word GPIO_FUNC0_OUT_SEL_CFG_REG + (BUILTIN_LED * 4)

#define BUILTIN_LED_MASK (1 << BUILTIN_LED)

.global init_builtin_led
.type init_builtin_led, @function
.align 4

init_builtin_led:
    entry   sp, 16
    
    l32r    a2, .Lgpio_enable
    movi.n  a3, BUILTIN_LED_MASK
    s32i.n  a3, a2, 0

    l32r    a2, .Lgpio_out
    movi.n  a3, 0
    s32i.n  a3, a2, 0
    
    l32r    a2, .Lgpio_func_cfg
    movi    a3, 256
    s32i.n  a3, a2, 0

    retw.n
    .size init_builtin_led, .-init_builtin_led

.global turn_on_builtin_led
.type turn_on_builtin_led, @function
.align 4

turn_on_builtin_led:
    entry   sp, 16

    l32r    a2, .Lgpio_out
    movi.n  a3, BUILTIN_LED_MASK
    l32i.n  a4, a2, 0 /* Deferences the address that a2 contains */
    or      a4, a4, a3
    s32i.n  a4, a2, 0   
    
    retw.n
    .size turn_on_builtin_led, .-turn_on_builtin_led

.global turn_off_builtin_led
.type turn_off_builtin_led, @function
.align 4

turn_off_builtin_led:
    entry   sp, 16

    l32r    a2, .Lgpio_out
    movi.n  a3, ~BUILTIN_LED_MASK
    l32i.n  a4, a2, 0 /* Deferences the address that a2 contains */
    and     a4, a4, a3
    s32i.n  a4, a2, 0

    retw.n
    .size turn_off_builtin_led, .-turn_off_builtin_led

/*This function is not used
.global turn_opposite_builtin_led
.type turn_opposite_builtin_led, @function
.align 4

turn_opposite_builtin_led:
    entry   sp, 16

    l32r    a2, .Lgpio_out
    movi.n  a3, BUILTIN_LED_MASK
    l32.n   a4, a2, 0
    xor     a4, a4, a3
    s32i.n  a4, a2, 0

    retw.n
    .size turn_opposite_builtin_led, -.turn_opposite_builtin_led

*/