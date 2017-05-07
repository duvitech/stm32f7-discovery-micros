#ifndef DFSDM_H
#define DFSDM_H

#include <stdint.h>

#define DFSDM_SAMPLE_LEN 10000

typedef void (*dfsdmcallback_t)(void *drv, int32_t *buffer, size_t n);
typedef void (*dfsdmerrorcallback_t)(void *drv);

/** Configure the hardware peripherals. */
void dfsdm_init(void);

/** Starts the continous acquisition. */
void dfsdm_start(void);

/** Stops the continous acquisition. */
void dfsdm_stop(void);

void dfsdm_left_set_callbacks(dfsdmcallback_t data_cb, dfsdmerrorcallback_t err_cb, void *arg);

/** Wait for a complete capture and returns a pointer to the buffer.
 *
 * @warning This is only part of the testing API and will be removed when DMA
 * support is complete. */
int32_t *dfsdm_wait(void);

#endif
