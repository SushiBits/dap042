/**
 * @file main.c
 * 
 * Main entry point of dap042
 */

#include "stm32f0xx.h"

void SystemInit(void) {
    // Set up HSE
    // HSE is a Murata CERALOCK tight-tolerance ceramic resonator: cheap,
    // 3.2 x 1.3 mm, built-in load caps (fewer BOM lines), and tight enough
    // for the USB 2.0 full-speed clock tolerance (+/-0.25%).
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
    for (;;) __WFE(); // Idle loop
}