/**
 * @file stm32f0xx_it.c
 * 
 * Bootup file for STM32F0xx.
 */

#include "stm32f0xx_it.h"
#include "stm32f0xx.h"
#include <string.h>

void __stack(void); // Stack pointer, to be palced by linker script.
void _start(void); // C runtime entry point.
void Reset_IRQHandler(void);
static void Default_IRQHandler(void);

__attribute__((used, section(".vector"))) vector_t vector[] = {
    [0] = __stack,
    [1] = Reset_IRQHandler,
#define IRQN_HANDLER(n, x) [n + 16] = x##_IRQHandler,
#define SKIP_HANDLER(n) [n + 16] = Default_IRQHandler,
#include "stm32f0xx_handlers.h"
#undef IRQN_HANDLER
#undef SKIP_HANDLER
};

#define IRQN_HANDLER(n, x) __attribute__((weak, alias("Default_IRQHandler"))) void x##_IRQHandler(void);
#define SKIP_HANDLER(n)
#include "stm32f0xx_handlers.h"
#undef IRQN_HANDLER
#undef SKIP_HANDLER

extern uint8_t __vector_start__;
extern uint8_t __vector_load__;
extern uint8_t __vector_end__;

extern uint8_t __data_start__;
extern uint8_t __data_load__;
extern uint8_t __data_end__;

extern uint8_t __bss_start__;
extern uint8_t __bss_end__;

void Reset_IRQHandler(void) {

    SystemInit(); // Initialize system clock

    memcpy(&__vector_start__, &__vector_load__, &__vector_end__ - &__vector_start__);
    memcpy(&__data_start__, &__data_load__, &__data_end__ - &__data_start__);
    memset(&__bss_start__, 0, &__bss_end__ - &__bss_start__);

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE_Msk, 0b11 << SYSCFG_CFGR1_MEM_MODE_Pos);

    SystemCoreClockUpdate();

    _start();

    for (;;) __WFE();
}

static void Default_IRQHandler(void) {
    __BKPT();
    for (;;) __WFE();
}

__attribute__((weak, alias("Default_IRQHandler"))) void _exit(int);
