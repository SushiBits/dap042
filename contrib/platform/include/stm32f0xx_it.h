/**
 * @file stm32f0xx_it.h
 * 
 * Bootup file for STM32F0 family
 */

#ifndef _STM32F0xx_IT_H_
#define _STM32F0xx_IT_H_

#include <sys/cdefs.h>

typedef void (*vector_t)(void);

__BEGIN_DECLS

extern vector_t vector[];

#define IRQN_HANDLER(x, n) void n##_IRQHandler(void);
#define SKIP_HANDLER(x)
#include "stm32f0xx_handlers.h"
#undef IRQN_HANDLER
#undef SKIP_HANDLER

__END_DECLS

#endif // defined _STM32F0xx_IT_H_