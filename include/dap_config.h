// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _DAP_CONFIG_H_
#define _DAP_CONFIG_H_

/*- Includes ----------------------------------------------------------------*/
#include "stm32f0xx.h"
#include "usbdefs.h"

/*- Definitions -------------------------------------------------------------*/
#define DAP_CONFIG_ENABLE_JTAG

#define DAP_CONFIG_DEFAULT_PORT        DAP_PORT_SWD
#define DAP_CONFIG_DEFAULT_CLOCK       1000000 // Hz

#define DAP_CONFIG_PACKET_SIZE         USB_EPD_SIZE
#define DAP_CONFIG_PACKET_COUNT        1

#define DAP_CONFIG_JTAG_DEV_COUNT      8

// DAP_CONFIG_PRODUCT_STR must contain "CMSIS-DAP" to be compatible with the standard
#define DAP_CONFIG_VENDOR_STR          USB_STRING_VENDOR
#define DAP_CONFIG_PRODUCT_STR         USB_STRING_PRODUCT
#define DAP_CONFIG_CMSIS_DAP_VER_STR   "2.0.0"

//#define DAP_CONFIG_RESET_TARGET_FN     target_specific_reset_function
//#define DAP_CONFIG_VENDOR_FN           vendor_command_handler_function

// Attribute to use for performance-critical functions
#define DAP_CONFIG_PERFORMANCE_ATTR

// A value at which dap_clock_test() produces 1 kHz output on the SWCLK pin
#define DAP_CONFIG_DELAY_CONSTANT      5800

// A threshold for switching to fast clock (no added delays)
// This is the frequency produced by dap_clock_test(1) on the SWCLK pin
#define DAP_CONFIG_FAST_CLOCK          3100000

/*- Prototypes --------------------------------------------------------------*/

/*- GPIO helpers ------------------------------------------------------------*/

/**
 * @defgroup dap_config_gpio GPIO helpers
 *
 * Minimal pin access for the bit-banged SWD/JTAG interface, built directly on
 * the CMSIS register blocks so that each edge costs a single store.
 *
 * Pins are given as a packed pin number of the form 0xPN: the high nibble
 * selects the port (0 = GPIOA, any other value = GPIOB) and the low nibble the
 * pin within that port, so PIN_TCK (0x05) is PA5 and PIN_NSRST (0x11) is PB1.
 * All helpers expect @p pin to be a compile-time constant.
 *
 * @{
 */

/**
 * @brief Resolve the GPIO port that owns a packed pin number.
 *
 * @param[in] pin Packed pin number (0xPN).
 * @return Pointer to the port register block, GPIOA for 0x0N, GPIOB for 0x1N.
 */
#define GPIO(pin) ((GPIO_TypeDef *)((pin & 0xF0) ? GPIOB : GPIOA))

/**
 * @brief Set the direction of a pin by writing its two MODER bits.
 *
 * MODER holds two bits per pin, so the shift is twice the pin index. The mask
 * is cleared before the mode is merged in, leaving the neighbouring pins
 * untouched.
 *
 * @param[in] pin  Packed pin number (0xPN).
 * @param[in] mode One of #INPUT or #OUTPUT.
 */
#define pinMode(pin, mode) MODIFY_REG(GPIO(pin)->MODER, 0b11 << ((pin & 0x0F) << 1), (mode & 0b11) << ((pin & 0x0F) << 1))

/**
 * @brief Drive a pin high or low with a single write to BSRR.
 *
 * BSRR is used instead of a read-modify-write of ODR, so the transition is
 * atomic and cannot be lost if the level is changed from an interrupt. The
 * upper half of the register resets the pin, hence the +16 offset.
 *
 * @param[in] pin   Packed pin number (0xPN).
 * @param[in] value Non-zero drives the pin high, zero drives it low.
 */
#define digitalWrite(pin, value) (GPIO(pin)->BSRR = 0b1 << ((pin & 0x0F) + (value ? 0 : 16)))

/**
 * @brief Sample the input level of a pin.
 *
 * @param[in] pin Packed pin number (0xPN).
 * @return 1 if the pin reads high, 0 if it reads low.
 */
#define digitalRead(pin) !!(GPIO(pin)->IDR & (0b1 << (pin & 0x0F)))

/**
 * @name SWD/JTAG pin assignments
 * Target debug port connections of the SUSHiBITS One Pro, as packed pin
 * numbers.
 * @{
 */
#define PIN_TCK 0x05   ///< SWCLK/TCK, PA5.
#define PIN_TMS 0x04   ///< SWDIO/TMS, PA4.
#define PIN_TDI 0x07   ///< TDI, PA7.
#define PIN_TDO 0x06   ///< TDO, PA6.
#define PIN_NSRST 0x11 ///< nRESET, PB1.
/** @} */

/**
 * @name Pin modes
 * Values accepted by the @p mode argument of pinMode(); they map onto the two
 * least significant bits of MODER.
 * @{
 */
#define INPUT 0b00  ///< Input, high impedance.
#define OUTPUT 0b01 ///< General purpose push-pull output.
/** @} */

/** @} */

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
static inline void DAP_CONFIG_DISCONNECT(void);
static inline void DAP_CONFIG_SETUP(void) {
  GPIOA->OSPEEDR |= 0x0000FF00; // High speed on TMS, TCK, TDI and TDO
  MODIFY_REG(GPIOA->PUPDR, 0x00000300, 0x00000100); // TMS pull-up
  GPIOB->OSPEEDR |= 0x0000000C; // High speed on nSRST
  GPIOB->OTYPER |= 0x0002; // nSRST OD
  // (There is an onboard 10k PU AND an internal PU in taget chip, so no need to tri-plicate that.)
  DAP_CONFIG_DISCONNECT();
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_DISCONNECT(void) {
  pinMode(PIN_TMS, INPUT);
  pinMode(PIN_TCK, INPUT);
  pinMode(PIN_TDI, INPUT);
  pinMode(PIN_TDO, INPUT);
  pinMode(PIN_NSRST, INPUT);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_CONNECT_SWD(void) {
  digitalWrite(PIN_TMS, 1);     /* level first: no glitch on the mode switch */
  digitalWrite(PIN_TCK, 1);
  digitalWrite(PIN_NSRST, 1);
  pinMode(PIN_TMS, OUTPUT);
  pinMode(PIN_TCK, OUTPUT);
  pinMode(PIN_NSRST, OUTPUT);
  pinMode(PIN_TDI, INPUT);      /* JTAG-only signals idle high-Z under SWD */
  pinMode(PIN_TDO, INPUT);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_CONNECT_JTAG(void) {
  DAP_CONFIG_CONNECT_SWD();
  digitalWrite(PIN_TDI, 1);
  pinMode(PIN_TDI, OUTPUT);
}

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

