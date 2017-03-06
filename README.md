# STM32F769 demonstrator

## Clock configuration

From `mcuconf.h` (check there for latest version):

```cpp
/* External oscillator is 25 MHz.
 * The following defines configure the clock system for:
 *
 * - VCO clock: 25 * PLLN / PLLM = 432.0 Mhz
 * - PLL output: VCO / PLLP = 216 Mhz
 * - PLL output for USB, RNG and SDMMC: VCO / PLLQ = 48 Mhz
 * - AHB clock (CPU, memory and DMA): PLL output / STM32_HPRE = 216 Mhz
 * - APB1 clock: PLL output / PPRE1 = 54 Mhz
 * - APB2 clock: PLL output / PPRE2 = 108 Mhz
 *
 *   See reference manual section 5.3.2 (RCC PLL configuration register) for
 *   details.
 */
```
