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

## Sound capture demo

This repository contains a small demo application for the DFSDM microphone.
It is invoked through the `capture_sound.py` script.
This script records a mono WAV file using one of the discovery board built in
DFSDM microphone.
Its basic usage is like this:

```
# Record 100 000 samples (about 2.5 seconds) of audio in test.wav
$ ./capture_sound.py /dev/tty.usbmodem401  test.wav -l 100000
```

See `capture_sound.py -h` for the complete list of parameters.
