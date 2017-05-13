#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <hal.h>
#include <ch_test.h>
#include <chprintf.h>
#include <shell.h>

#include "usbconf.h"
#include "dfsdm.h"

#define SHELL_WA_SIZE 2048
#define AUDIO_BUFFER_SIZE 1000

BSEMAPHORE_DECL(data_ready, true);
static int32_t *samples;

static void cmd_reboot(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    (void) chp;
    NVIC_SystemReset();
}

static void dfsdm_err_cb(void *p)
{
    (void) p;
    chSysHalt("DFSDM DMA error");
}

static void dfsdm_data_callback(void *p, int32_t *buffer, size_t n)
{
    (void) n;
    (void) buffer;

    /* Only a half buffer is used at a time. This means that while we are
     * processing one half of the buffer, the other half already captures the
     * new data. */
    osalDbgAssert(n == AUDIO_BUFFER_SIZE / 2, "Buffer size is invalid.");

    /* Check if it is the microphone we are using. */
    if ((int) p) {
        chSysLockFromISR();
        samples = buffer;
        chBSemSignalI(&data_ready);
        chSysUnlockFromISR();
    }
}

static int32_t left_buffer[AUDIO_BUFFER_SIZE];
static DFSDM_config_t left_cfg = {
    .end_cb = dfsdm_data_callback,
    .error_cb = dfsdm_err_cb,
    .samples = left_buffer,
    .samples_len = AUDIO_BUFFER_SIZE
};

static int32_t right_buffer[AUDIO_BUFFER_SIZE];
static DFSDM_config_t right_cfg = {
    .end_cb = dfsdm_data_callback,
    .error_cb = dfsdm_err_cb,
    .samples = right_buffer,
    .samples_len = AUDIO_BUFFER_SIZE
};

static void cmd_dfsdm(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    if (argc != 1) {
        chprintf(chp, "Usage: dfsdm left|right\r\n");
        return;
    }

    /* We use the callback arg to store which microphone is used. */
    if (!strcmp(argv[0], "left")) {
        left_cfg.cb_arg = (void*) 1;
        right_cfg.cb_arg = (void*) 0;
    } else {
        left_cfg.cb_arg = (void*) 0;
        right_cfg.cb_arg = (void*) 1;
    }

    dfsdm_start_conversion(&left_cfg, &right_cfg);

    chprintf(chp, "Done !\r\n");

    /* High pass filter params */
    const float tau = 1 / 20.; /* 1 / cutoff */
    const float dt = 1./44e3;  /* Sampling period */

    const float alpha = tau / (tau + dt);

    int32_t x_prev = 0, y = 0, x;

    while (true) {
        chBSemWait(&data_ready);
        size_t i;

        /* First order high pass filtering is used to remove the DC component
         * of the signal. */
        for (i = 0; i < AUDIO_BUFFER_SIZE / 2; i++) {
            x = samples[i];
            y = alpha * y + alpha * (x - x_prev);
            x_prev = x;
            samples[i] = y;
        }

        streamWrite(chp, (uint8_t *)samples, sizeof(left_buffer) / 2);
    }
}

static ShellCommand shell_commands[] = {
    {"reboot", cmd_reboot},
    {"dfsdm", cmd_dfsdm},
    {NULL, NULL}
};

/** Helper function used to sort the commands by name. */
static int compare_shell_commands(const void *ap, const void *bp)
{
    const ShellCommand *a = (ShellCommand *)ap;
    const ShellCommand *b = (ShellCommand *)bp;

    return strcmp(a->sc_name, b->sc_name);
}

static THD_FUNCTION(shell_spawn_thd, p)
{
    (void) p;
    thread_t *shelltp = NULL;

    static ShellConfig shell_cfg = {
        (BaseSequentialStream *)&SDU1,
        shell_commands
    };

    /* Sort the commands by name. */
    qsort(shell_commands,
          /* Ignore the last element, which is NULL. */
          (sizeof(shell_commands) / sizeof(ShellCommand)) - 1,
          sizeof(ShellCommand),
          compare_shell_commands);

    shellInit();

    while (TRUE) {
        if (!shelltp) {
            if (SDU1.config->usbp->state == USB_ACTIVE) {
                shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                              "shell", NORMALPRIO + 1,
                                              shellThread, (void *)&shell_cfg);
            }
        } else {
            if (chThdTerminatedX(shelltp)) {
                chThdRelease(shelltp);
                shelltp = NULL;
            }
        }
        chThdSleepMilliseconds(500);
    }
}


void shell_start(void)
{
    static THD_WORKING_AREA(wa, SHELL_WA_SIZE);

    chThdCreateStatic(wa, sizeof(wa), NORMALPRIO, shell_spawn_thd, NULL);
}
