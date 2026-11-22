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
    // HSE is a 12 MHz crystal. On this branch it clocks SYSCLK only - USB runs
    // from HSI48, trimmed by the CRS against the host's SOF frames - so its
    // absolute accuracy no longer has to meet the +/-0.25% USB budget. The
    // board still uses a crystal rather than a resonator: the difference in
    // cost is small, and it keeps the PLL-clocked USB variant a one-line
    // change to compare against.
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
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP; // USB_DM/USB_DP on PA11/PA12.

    // Run USB from HSI48 instead of the PLL, and discipline HSI48 to the host
    // rather than to our own crystal: the CRS compares HSI48 against the 1 kHz
    // SOF frames and trims it to them, so the PHY tracks the host's clock. The
    // PLL is left free to clock SYSCLK and everything else, and the two
    // domains no longer share a source.
    RCC->CR2 |= RCC_CR2_HSI48ON;
    while (!(RCC->CR2 & RCC_CR2_HSI48RDY));
    RCC->APB1ENR |= RCC_APB1ENR_CRSEN; // CRS clock, before the first write to it
    MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USBSW_Msk, RCC_CFGR3_USBSW_HSI48); // USB clock = HSI48

    // CRS_CFGR resets to SYNCSRC = USB SOF, RELOAD = 47999 (one 1 kHz SOF
    // period measured at 48 MHz) and FELIM = 0x22, which is exactly the window
    // this needs, so only the counter has to be started. Leaving it running
    // while the device waits to enumerate costs nothing: until the first SOF
    // arrives the counter runs untrimmed and TRIM holds its factory value, and
    // while the bus is suspended it holds the last correction.
    CRS->CR = CRS_CR_AUTOTRIMEN | CRS_CR_CEN;
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