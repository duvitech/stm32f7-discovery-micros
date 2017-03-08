#include <ch.h>
#include <hal.h>

#include "usbconf.h"
#include "cmd.h"

void blink_start(void);


int main(void)
{
    halInit();
    chSysInit();

    blink_start();

    /* Starts the virtual serial port. */
    usb_start();
    shell_start();

    while (true) {
        chThdSleepMilliseconds(500);
    }
}

static THD_FUNCTION(blink, arg)
{
    (void) arg;
    chRegSetThreadName(__FUNCTION__);

    while (true) {
        palSetLine(LINE_LED2_GREEN);
        chThdSleepMilliseconds(500);
        palClearLine(LINE_LED2_GREEN);
        chThdSleepMilliseconds(500);
    }
}

/** Starts the heartbeat thread */
void blink_start(void)
{
    static THD_WORKING_AREA(wablink, 128);
    chThdCreateStatic(wablink, sizeof(wablink), NORMALPRIO, blink, NULL);
}
