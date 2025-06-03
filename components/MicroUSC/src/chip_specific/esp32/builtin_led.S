#include "soc/gpio_reg.h"

#define BUILTIN_LED   2

.section .text
.align 4

.global init_builtin_led
.type init_builtin_led, @function

init_builtin_led:
    entry   sp, 16
    
    movi    a2, GPIO_ENABLE_REG
    movi.n  a3, (1 << BUILTIN_LED)
    s32i    a3, a2, 0

    movi    a2, GPIO_OUT_REG
    movi.n  a3, 0
    s32i    a3, a2, 0
    
    movi    a2, GPIO_FUNC0_OUT_SEL_CFG_REG + (BUILTIN_LED * 4)
    movi    a3, 256
    s32i    a3, a2, 0

    retw.n
    .size init_builtin_led, .-init_builtin_led
    .align 4

.global turn_on_builtin_led
.type turn_on_builtin_led, @function

turn_on_builtin_led:
    entry   sp, 16

    movi    a2, GPIO_OUT_REG
    movi.n  a3, (1 << BUILTIN_LED)
    l32i    a4, a2, 0
    or      a4, a4, a3
    s32i    a4, a2, 0   
    
    retw.n
    .size turn_on_builtin_led, .-turn_on_builtin_led
    .align 4

.global turn_off_builtin_led
.type turn_off_builtin_led, @function

turn_off_builtin_led:
    entry   sp, 16

    movi    a2, GPIO_OUT_REG
    movi.n  a3, ~(1 << BUILTIN_LED)
    l32i    a4, a2, 0
    and     a4, a4, a3
    s32i    a4, a2, 0

    retw.n
    .size turn_off_builtin_led, .-turn_off_builtin_led
    .align 4

/* constructors */
/* this code does not work as intneded (contructor)
.section .init_array, "aw", @init_array
.align 4
.long init_builtin_led
*/