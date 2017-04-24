#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <hal.h>
#include <ch_test.h>
#include <chprintf.h>
#include <shell.h>

#include "usbconf.h"

#define SHELL_WA_SIZE 2048

/* Those defines are missing from the STM32F769 include, copied them from the L4 one. */
#define DFSDM_CHCFGR1_CKOUTDIV_Pos           (16U)
#define DFSDM_CHCFGR1_CKOUTDIV_Msk           (0xFFU << DFSDM_CHCFGR1_CKOUTDIV_Pos) /*!< 0x00FF0000 */
#define DFSDM_FLTCR1_RCH_Pos                 (24U)
#define DFSDM_FLTFCR_IOSR_Pos                (0U)
#define DFSDM_FLTFCR_FOSR_Pos                (16U)
#define DFSDM_FLTFCR_FORD_Pos                (29U)

static void cmd_reboot(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    (void) chp;
    NVIC_SystemReset();
}

OSAL_IRQ_HANDLER(Vector1CC) {

    OSAL_IRQ_PROLOGUE();

    /* For now just break in debugger. */
    __asm__("bkpt");

    OSAL_IRQ_EPILOGUE();
}


static void cmd_dfsdm(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    chprintf(chp, "Configuring DFSDM peripheral... ");

    /* Send clock to peripheral. */
    rccEnableAPB2(RCC_APB2ENR_DFSDM1EN, true);

    /* Configure DFSDM clock output (must be before enabling interface).
     *
     * The clock output is used by the microphones to send their data out.
     * DFSDM is on APB2 @ 108 Mhz. The MP34DT01 MEMS microphone runs @ 2.4 Mhz,
     * requiring a prescaler of 45.
     */
    const unsigned clkout_div = 45;
    DFSDM1_Channel0->CHCFGR1 |= (clkout_div & 0xff) << DFSDM_CHCFGR1_CKOUTDIV_Pos;

    /* Enable DFSDM interface */
    DFSDM1_Channel0->CHCFGR1 |= DFSDM_CHCFGR1_DFSDMEN;

    /* Serial input configuration.
     *
     * The two microphones (left and right) are connected to the same input pin.
     * As the microphone dont have a clock output, we reuse the internal clock.
     *
     * Channel 0 is connected on the input from channel 1 (CHINSEL=1)
     * Channel 0 data are on rising edge (SITP=0), while channel 1 are on falling edge(SITP=1).
     */
    DFSDM1_Channel0->CHCFGR1 |= DFSDM_CHCFGR1_CHINSEL;
    DFSDM1_Channel0->CHCFGR1 |= DFSDM_CHCFGR1_SPICKSEL_1;

    DFSDM1_Channel1->CHCFGR1 |= DFSDM_CHCFGR1_SPICKSEL_1;
    DFSDM1_Channel1->CHCFGR1 |= DFSDM_CHCFGR1_SITP_1;

    /* Enable channel 0 and 1. */
    DFSDM1_Channel0->CHCFGR1 |= DFSDM_CHCFGR1_CHEN;
    DFSDM1_Channel1->CHCFGR1 |= DFSDM_CHCFGR1_CHEN;

    /* Filter units configuration:
     * - Fast mode enabled
     * - Corresponding channel must be selected
     * - Continuous mode
     * - For channel 1: start synchronously with channel 0
     * - Sinc3 filter (from ST application example)
     * - Oversampling factor (OF) = 55, integrator oversampling (IF) = 1
     *   -> acquisition rate = APB2 / (clkout_div * OF * IF)
     *                       = 108 Mhz / (45 * 55) = 43.6 Khz.
     *   -> resolution = +/- (OF)^3
     *                 = +/- 166375
     *                 = ~ 19 bits (including sign bit)
     *    For details on filter configuration see section 17.3.8 of the
     *    reference manual (Digital Filter Configuration).
     *
     * TODO: Add DMA/IRQ support
     * TODO: Get to a precise 44.1 Khz clock using audio PLL
     */
    DFSDM1_Filter0->FLTCR1 = DFSDM_FLTCR1_FAST \
                             | DFSDM_FLTCR1_RCONT \
                             | (0 << DFSDM_FLTCR1_RCH_Pos);     /* channel */
    DFSDM1_Filter0->FLTFCR = (3 << DFSDM_FLTFCR_FORD_Pos)       /* filter order */ \
                             | (55 << DFSDM_FLTFCR_FOSR_Pos)    /* filter oversampling */ \
                             | (01 << DFSDM_FLTFCR_IOSR_Pos);   /* integrator oversampling */

    /* Filter 1 is identical, except that RSYNC is enabled. */
    DFSDM1_Filter1->FLTCR1 = DFSDM_FLTCR1_FAST \
                             | DFSDM_FLTCR1_RCONT \
                             | DFSDM_FLTCR1_RSYNC \
                             | (0 << DFSDM_FLTCR1_RCH_Pos);     /* channel */
    DFSDM1_Filter1->FLTFCR = (3 << DFSDM_FLTFCR_FORD_Pos)       /* filter order */ \
                             | (55 << DFSDM_FLTFCR_FOSR_Pos)    /* filter oversampling */ \
                             | (01 << DFSDM_FLTFCR_IOSR_Pos);   /* integrator oversampling */


    /* Enable the filters */
    DFSDM1_Filter0->FLTCR1 |= DFSDM_FLTCR1_DFEN;
    DFSDM1_Filter1->FLTCR1 |= DFSDM_FLTCR1_DFEN;

    chprintf(chp, " [OK]\r\n");

    chprintf(chp, "Starting acquisition...\r\n");

    /* Enable interrupts coming from filter unit 0 for testing. */
    nvicEnableVector(DFSDM1_FLT0_IRQn, 6);

    /* Enable conversion done interrupt. */
    DFSDM1_Filter0->FLTCR2 |= DFSDM_FLTCR2_REOCIE;

    /* Start acquisition */
    DFSDM1_Filter0->FLTCR1 |= DFSDM_FLTCR1_RSWSTART;
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
