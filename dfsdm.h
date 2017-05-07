#ifndef DFSDM_H
#define DFSDM_H

#include <stdint.h>
#include <unistd.h>

typedef void (*dfsdmcallback_t)(void *drv, int32_t *buffer, size_t n);
typedef void (*dfsdmerrorcallback_t)(void *drv);

typedef struct {
    dfsdmcallback_t end_cb;
    dfsdmerrorcallback_t error_cb;
    void *cb_arg;
    int32_t *samples;
    size_t samples_len;
} DFSDM_config_t;

/** Configure the hardware peripherals. */
void dfsdm_init(void);

/** Starts the continous acquisition. */
void dfsdm_start(DFSDM_config_t *left_config, DFSDM_config_t *right_config);

/** Stops the continous acquisition. */
void dfsdm_stop(void);

#endif
