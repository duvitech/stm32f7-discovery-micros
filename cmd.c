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
    (void) p;
    (void) n;
    (void) buffer;
    chSysLockFromISR();
    samples = buffer;
    chBSemSignalI(&data_ready);
    chSysUnlockFromISR();
}

static int32_t left_buffer[1000];
static DFSDM_config_t left_cfg = {
    .end_cb = dfsdm_data_callback,
    .error_cb = dfsdm_err_cb,
    .samples = left_buffer,
    .samples_len = sizeof(left_buffer) / sizeof(int32_t)
};

static int32_t right_buffer[1000];
static DFSDM_config_t right_cfg = {
    .end_cb = dfsdm_data_callback,
    .error_cb = dfsdm_err_cb,
    .samples = right_buffer,
    .samples_len = sizeof(right_buffer) / sizeof(int32_t)
};


static void cmd_dfsdm(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    chprintf(chp, "Starting acquisition...\r\n");
    chprintf(chp, "Done !\r\n");

    dfsdm_start(&left_cfg, &right_cfg);

    while (true) {
        chBSemWait(&data_ready);
        streamWrite(chp, (uint8_t *)samples, sizeof(left_buffer) / 2);
    }

    dfsdm_stop();

    chThdSleepMilliseconds(2000);
    NVIC_SystemReset();
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
