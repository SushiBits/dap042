/**
 * @file main.c
 * 
 * Main entry point of dap042
 */

#include "stm32f0xx.h"

#include "usbmain.h"
#include "usbhid.h"
#include "dap.h"

#include <string.h>

void SystemInit(void) {
    // Set up HSE
    // HSE is a 12 MHz crystal. Use a crystal here, not a ceramic resonator:
    // USB full-speed needs the 48 MHz clock within +/-0.25% including drift,
    // and a resonator that misses it fails as CRC errors on every packet
    // rather than as a clean fault.
    RCC->CR |= RCC_CR_HSEON; // HSE = 12MHz.
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Set up PLL
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC_Msk, RCC_CFGR_PLLSRC_HSE_PREDIV); // PLLCLK = 12MHz...
    RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV1; // ... / 1...
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLMUL_Msk, RCC_CFGR_PLLMUL4); // ... * 4 = 48MHz
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Set up Flash latency
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY_Msk, 1 << FLASH_ACR_LATENCY_Pos); // 1 WS
    FLASH->ACR |= FLASH_ACR_PRFTBE; // With prefetch.

    // Migrate to PLL clock
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW_Msk, RCC_CFGR_SW_PLL); // SYSCLK = PLLCLK
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);

    // Turn on important peripherals
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
    MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USBSW_Msk, RCC_CFGR3_USBSW_PLLCLK); // USB clock = PLLCLK
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP; // USB_DM/USB_DP on PA11/PA12.
}

int main(void) {
    usb_init();
    dap_init();
    //dap_clock_test(6000); for (;;) __WFE();

    for (;;) {
        if (rx_drdy && !tx_drdy) {
            if (dap_filter_request(rx_data)) {
                memset(tx_data, 0, hid_size);
                dap_process_request(rx_data, hid_size, tx_data, hid_size); // HID. Fixed size responses.

                // dap_process_request() reports an overflow through
                // dap_is_buf_error() rather than through its return value, and
                // by then it has already written part of a response. The host
                // parses the report according to the command, so a truncated
                // one reads as data - a Transfer that claims more results than
                // it carries. Replace the whole report with the shortest
                // response that is valid for this command.
                if (dap_is_buf_error()) {
                    memset(tx_data, 0, hid_size);
                    dap_build_error_response(rx_data[0], tx_data, hid_size);
                }

                tx_drdy = true;
                rx_drdy = false;
                hid_data_tx();
                hid_data_rx();
            } else {
                rx_drdy = false;
                hid_data_rx();
            }
        } else {
            __WFE();
        }
    }
}