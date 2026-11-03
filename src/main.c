/**
 * @file main.c
 * 
 * Main entry point of dap042
 */

#include "stm32f0xx.h"

void SystemInit(void) {
    // Set up HSE
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Set up PLL
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC_Msk, RCC_CFGR_PLLSRC_HSE_PREDIV);
    RCC->CFGR2 = RCC_CFGR2_PREDIV_DIV1;
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLMUL_Msk, RCC_CFGR_PLLMUL4);
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Set up Flash latency
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY_Msk, 1 << FLASH_ACR_LATENCY_Pos);
    FLASH->ACR |= FLASH_ACR_PRFTBE;

    // Migrate to PLL clock
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW_Msk, RCC_CFGR_SW_PLL);
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);

    // Turn on important peripherals
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
    MODIFY_REG(RCC->CFGR3, RCC_CFGR3_USBSW_Msk, RCC_CFGR3_USBSW_PLLCLK);
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;
}

int main(void) {
    for (;;) __WFE(); // Idle loop
}