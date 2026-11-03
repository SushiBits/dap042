/**
 * @file stm32f0xx_handlers.h
 * 
 * Handlers X-Macro Redirector
 */

#if defined(STM32F030x6)
  #include "stm32f030x6_handlers.h"
#elif defined(STM32F030x8)
  #include "stm32f030x8_handlers.h"
#elif defined(STM32F031x6)
  #include "stm32f031x6_handlers.h"
#elif defined(STM32F038xx)
  #include "stm32f038xx_handlers.h"
#elif defined(STM32F042x6)
  #include "stm32f042x6_handlers.h"
#elif defined(STM32F048xx)
  #include "stm32f048xx_handlers.h"
#elif defined(STM32F051x8)
  #include "stm32f051x8_handlers.h"
#elif defined(STM32F058xx)
  #include "stm32f058xx_handlers.h"
#elif defined(STM32F070x6)
  #include "stm32f070x6_handlers.h"
#elif defined(STM32F070xB)
  #include "stm32f070xb_handlers.h"
#elif defined(STM32F071xB)
  #include "stm32f071xb_handlers.h"
#elif defined(STM32F072xB)
  #include "stm32f072xb_handlers.h"
#elif defined(STM32F078xx)
  #include "stm32f078xx_handlers.h"
#elif defined(STM32F091xC)
  #include "stm32f091xc_handlers.h"
#elif defined(STM32F098xx)
  #include "stm32f098xx_handlers.h"
#elif defined(STM32F030xC)
  #include "stm32f030xc_handlers.h"    
#else
 #error "Please select first the target STM32F0xx device used in your application (in stm32f0xx.h file)"
#endif