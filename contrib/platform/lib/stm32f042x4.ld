/**
 * @file stm32f042x4.ld
 *
 * Linker script snippet for STM32F042x4, indicating memory layout.
 */

MEMORY {
    IROM (r x) : ORIGIN = 0x08000000, LENGTH = 16k
    IRAM (rw ) : ORIGIN = 0x20000000, LENGTH = 6k
}
