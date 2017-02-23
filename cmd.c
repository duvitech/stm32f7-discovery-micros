#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <hal.h>
#include <ch_test.h>
#include <chprintf.h>
#include <shell.h>

#include "usbconf.h"

#define SHELL_WA_SIZE 2048

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    test_execute(chp);
}

static ShellCommand shell_commands[] = {
    {"test", cmd_test},
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
