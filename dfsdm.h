#ifndef DFSDM_H
#define DFSDM_H

#include <stdint.h>

#define DFSDM_SAMPLE_LEN 10000

/** Configure the hardware peripherals. */
void dfsdm_init(void);

/** Starts the continous acquisition. */
void dfsdm_start(void);

/** Stops the continous acquisition. */
void dfsdm_stop(void);

/** Wait for a complete capture and returns a pointer to the buffer.
 *
 * @warning This is only part of the testing API and will be removed when DMA
 * support is complete. */
int32_t *dfsdm_wait(void);

#endif
