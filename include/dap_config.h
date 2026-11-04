// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _DAP_CONFIG_H_
#define _DAP_CONFIG_H_

/*- Includes ----------------------------------------------------------------*/
#include "stm32f0xx.h"

/*- Definitions -------------------------------------------------------------*/
#define DAP_CONFIG_ENABLE_JTAG

#define DAP_CONFIG_DEFAULT_PORT        DAP_PORT_SWD
#define DAP_CONFIG_DEFAULT_CLOCK       1000000 // Hz

#define DAP_CONFIG_PACKET_SIZE         512
#define DAP_CONFIG_PACKET_COUNT        1

#define DAP_CONFIG_JTAG_DEV_COUNT      8

// DAP_CONFIG_PRODUCT_STR must contain "CMSIS-DAP" to be compatible with the standard
#define DAP_CONFIG_VENDOR_STR          "Alex Taradov"
#define DAP_CONFIG_PRODUCT_STR         "Generic CMSIS-DAP Adapter"
#define DAP_CONFIG_SER_NUM_STR         usb_serial_number
#define DAP_CONFIG_CMSIS_DAP_VER_STR   "2.0.0"

//#define DAP_CONFIG_RESET_TARGET_FN     target_specific_reset_function
//#define DAP_CONFIG_VENDOR_FN           vendor_command_handler_function

// Attribute to use for performance-critical functions
#define DAP_CONFIG_PERFORMANCE_ATTR

// A value at which dap_clock_test() produces 1 kHz output on the SWCLK pin
#define DAP_CONFIG_DELAY_CONSTANT      19000

// A threshold for switching to fast clock (no added delays)
// This is the frequency produced by dap_clock_test(1) on the SWCLK pin
#define DAP_CONFIG_FAST_CLOCK          8550000

/*- Prototypes --------------------------------------------------------------*/
extern char usb_serial_number[16];

#define GPIO(pin) ((GPIO_TypeDef *)((pin & 0xF0) ? GPIOB : GPIOA))
#define pinMode(pin, mode) MODIFY_REG(GPIO(pin)->MODER, 0b11 << ((pin & 0x0F) << 1), (mode & 0b11) << ((pin & 0x0F) << 1))
#define digitalWrite(pin, value) (GPIO(pin)->BSRR = 0b1 << ((pin & 0x0F) + (value ? 0 : 16)))
#define digitalRead(pin) !!(GPIO(pin)->IDR & (0b1 << (pin & 0x0F)))

#define PIN_TCK 0x05
#define PIN_TMS 0x04
#define PIN_TDI 0x07
#define PIN_TDO 0x06
#define PIN_NSRST 0x11

#define INPUT 0b00
#define OUTPUT 0b01

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWCLK_TCK_write(int value) { digitalWrite(PIN_TCK, value); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWDIO_TMS_write(int value) { digitalWrite(PIN_TMS, value); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_TDI_write(int value) { digitalWrite(PIN_TDI, value); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_TDO_write(int value) { digitalWrite(PIN_TDO, value); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_nTRST_write(int value) {}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_nRESET_write(int value) { digitalWrite(PIN_NSRST, value); }

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_SWCLK_TCK_read(void) { return digitalRead(PIN_TCK); }

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_SWDIO_TMS_read(void) { return digitalRead(PIN_TMS); }

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_TDO_read(void) { return digitalRead(PIN_TDO); }

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_TDI_read(void) { return digitalRead(PIN_TDI); }

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_nTRST_read(void) { return 1; }

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_nRESET_read(void) { return digitalRead(PIN_NSRST); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWCLK_TCK_set(void) { digitalWrite(PIN_TCK, 1); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWCLK_TCK_clr(void) { digitalWrite(PIN_TCK, 0); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWDIO_TMS_in(void) { pinMode(PIN_TMS, INPUT); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWDIO_TMS_out(void) { pinMode(PIN_TMS, OUTPUT); }

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SETUP(void) {}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_DISCONNECT(void) {}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_CONNECT_SWD(void) {}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_CONNECT_JTAG(void) {}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_LED(int index, int state) {}

//-----------------------------------------------------------------------------
__attribute__((always_inline))
static inline void DAP_CONFIG_DELAY(uint32_t cycles)
{
  asm volatile (
    "1: sub %[cycles], %[cycles], #1 \n"
    "   bne 1b \n"
    : [cycles] "+l"(cycles)
  );
}

#endif // _DAP_CONFIG_H_

