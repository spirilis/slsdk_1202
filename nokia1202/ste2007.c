/**
 * @file ste2007.c
 * @brief Nokia 1202 STE2007 TI Display Driver - Main driver code
 * @author Eric Brundick
 * @date 2018
 * @version 100
 *
 * @details Nokia 1202 TI Display driver API - Driver code
 *
 * @copyright (C) 2018 Eric Brundick spirilis at linux dot com
 *  @n Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files
 *  @n (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge,
 *  @n publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
 *  @n do so, subject to the following conditions:
 *  @n
 *  @n The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  @n
 *  @n THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *  @n OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *  @n BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 *  @n OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
// DPL (Driver Porting Layer) SystemP has a vsnprintf() implementation
#include <ti/drivers/dpl/SystemP.h>
#include <ti/drivers/dpl/SemaphoreP.h>

#include "spitxn.h"
#include "ste2007.h"
#include "font_5x7.h"


//! @brief Internal driver FxnTable prototypes
unsigned int ste2007_getType();
int ste2007_control_mutexwrapped(Display_Handle, unsigned int, void *);
int ste2007_control(Display_Handle, unsigned int, void *);
void ste2007_close(Display_Handle);
void ste2007_vprintf(Display_Handle, uint8_t, uint8_t, char *, va_list);

//! @brief Driver FxnTable
const Display_FxnTable DisplayNokia1202_FxnTable = {
/*
    Display_initFxn       initFxn;
    Display_openFxn       openFxn;
    Display_clearFxn      clearFxn;
    Display_clearLinesFxn clearLinesFxn;
    Display_vprintfFxn    vprintfFxn;
    Display_closeFxn      closeFxn;
    Display_controlFxn    controlFxn;
    Display_getTypeFxn    getTypeFxn;
 */
    .initFxn = ste2007_init,
    .openFxn = ste2007_open,
    .clearFxn = ste2007_clear,
    .clearLinesFxn = ste2007_clearLines,
    .vprintfFxn = ste2007_vprintf,
    .closeFxn = ste2007_close,
    .controlFxn = ste2007_control_mutexwrapped,
    .getTypeFxn = ste2007_getType
};

//! @brief Using TI-Drivers GPIO_write to set CS pin
void ste2007_chipselect(Display_Handle dpyH, uint8_t onoff)
{
    const DisplayNokia1202_HWAttrsV1 *h = dpyH->hwAttrs;

    GPIO_write(h->csPin, onoff);
}

//! @brief Send a simple 1-byte command
void ste2007_issuecmd(Display_Handle dpyH, uint8_t cmd, uint8_t arg, uint8_t argmask)
{
    DisplayNokia1202_Object *o = dpyH->object;

    uint8_t c = cmd | (arg & argmask);

    spitxn_reset(&(o->cmdBuf));
    spitxn_push(&(o->cmdBuf), 0, &c, 1);

    SPI_Transaction txn;
    txn.count = o->cmdBuf.len;
    txn.txBuf = (void *)(o->cmdBuf.buf);
    txn.rxBuf = 0;

    ste2007_chipselect(dpyH, 0);
    SPI_transfer(o->bus, &txn);
    ste2007_chipselect(dpyH, 1);
}

//! @brief Send a more complex 2-byte command
void ste2007_issue_compoundcmd(Display_Handle dpyH, uint8_t cmd, uint8_t arg, uint8_t argmask)
{
    uint8_t c[2];
    DisplayNokia1202_Object *o = dpyH->object;

    spitxn_reset(&(o->cmdBuf));
    c[0] = cmd;
    c[1] = arg & argmask;
    spitxn_push(&(o->cmdBuf), 0, c, 2);

    SPI_Transaction txn;
    txn.count = o->cmdBuf.len;
    txn.txBuf = (void *)(o->cmdBuf.buf);
    txn.rxBuf = 0;

    ste2007_chipselect(dpyH, 0);
    SPI_transfer(o->bus, &txn);
    ste2007_chipselect(dpyH, 1);
}

//! @brief TI Display_init() handler - can run outside of RTOS runtime
void ste2007_init(Display_Handle dpyH)
{
    DisplayNokia1202_Object *o = dpyH->object;

    o->cmdBuf.buf = o->_cmdBuffer;
    o->cmdBuf.cap = 4;
    o->cmdBuf.len = 0;
    o->rowbuffer.buf = o->_rowBuf;
    o->rowbuffer.cap = 16*6;
    o->rowbuffer.len = 0;
}

/** @brief Logical LCD operations
 */

//! @brief TI Display_open() handler - must run within an RTOS thread
//! @return Same value as dpyH if successful and NULL if a failure occurred in opening the display
Display_Handle ste2007_open(Display_Handle dpyH, Display_Params *params)
{
    DisplayNokia1202_Object *o = dpyH->object;
    const DisplayNokia1202_HWAttrsV1 *h = dpyH->hwAttrs;
    SPI_Params spiP;

    GPIO_init();
    SPI_init();

    // Init mutex
    o->mutex = SemaphoreP_createBinary(1);
    if (o->mutex == NULL) {
        System_printf("SemaphoreP_createBinary failed!\n");
        System_flush();
        return NULL;
    }

    // Grab mutex and continue initialization
    SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);


    GPIO_setConfig(h->csPin, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);  // CS in HIGH (off) position
    if (h->useBacklight) {
        GPIO_setConfig(h->backlightPin, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW); // Backlight in LOW (off) position
    }

    SPI_Params_init(&spiP);
    spiP.transferMode = SPI_MODE_BLOCKING;
    spiP.transferTimeout = SPI_WAIT_FOREVER;
    spiP.mode = SPI_MASTER;
    spiP.dataSize = 9;
    spiP.bitRate = 1000000;  // 1MHz
    spiP.frameFormat = SPI_POL0_PHA0;  // Mode 0
    o->bus = SPI_open(h->spiBus, &spiP);
    if (o->bus == NULL) {
        //SemaphoreP_post(o->mutex);
        return NULL;
    }

    ste2007_issuecmd(dpyH, STE2007_CMD_RESET, 0, STE2007_MASK_RESET);  // Soft RESET
    ste2007_issuecmd(dpyH, STE2007_CMD_DPYALLPTS, 0, STE2007_MASK_DPYALLPTS); // Powersave ALLPOINTS-ON mode turned OFF
    ste2007_issuecmd(dpyH, STE2007_CMD_PWRCTL, 7, STE2007_MASK_PWRCTL); // Power control set to max
    ste2007_issuecmd(dpyH, STE2007_CMD_ONOFF, 1, STE2007_MASK_ONOFF); // Display ON
    ste2007_issuecmd(dpyH, STE2007_CMD_COMDIR, 0, STE2007_MASK_COMDIR); // Display common driver = NORMAL
    ste2007_issuecmd(dpyH, STE2007_CMD_SEGMENTDIR, 0, STE2007_MASK_SEGMENTDIR); // Lines start at the left
    ste2007_issuecmd(dpyH, STE2007_CMD_ELECTVOL, 16, STE2007_MASK_ELECTVOL); // Electronic volume set to 16

    // Need to release the mutex since ste2007_clear() uses it
    SemaphoreP_post(o->mutex);
    ste2007_clear(dpyH);
    SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);

    ste2007_issue_compoundcmd(dpyH, STE2007_CMD_REFRESHRATE, 3, STE2007_MASK_REFRESHRATE); // Refresh rate = 65Hz
    ste2007_issue_compoundcmd(dpyH, STE2007_CMD_CHARGEPUMP, 0, STE2007_MASK_CHARGEPUMP); // Charge Pump multiply factor = 5x
    ste2007_issuecmd(dpyH, STE2007_CMD_SETBIAS, 6, STE2007_MASK_SETBIAS); // Bias ratio = 1/4
    ste2007_issue_compoundcmd(dpyH, STE2007_CMD_VOP, 0, STE2007_MASK_VOP);
    ste2007_issuecmd(dpyH, STE2007_CMD_DPYREV, 0, STE2007_MASK_DPYREV); // Display normal (not inverted)

    o->lineClearMode = params->lineClearMode;

    // Release mutex
    SemaphoreP_post(o->mutex);

    return dpyH;
}


//! @brief Fully erase DDRAM - TI Display_clear() handler
void ste2007_clear(Display_Handle dpyH)
{
    int i;
    DisplayNokia1202_Object *o = dpyH->object;

    SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);

    for(i=0; i < 16*6; i++) {
        o->_rowBuf[i] = 0x0100;
    }
    o->rowbuffer.len = 16*6;

    SPI_Transaction txn;
    txn.count = o->rowbuffer.len;
    txn.txBuf = (void *)(o->rowbuffer.buf);
    txn.rxBuf = (void *)0;

    ste2007_setxy(dpyH, 0, 0);
    ste2007_chipselect(dpyH, 0);
    for (i=0; i < 9; i++) {  // Each SPI_transfer writes 1 full row, do this 9 times.
        SPI_transfer(o->bus, &txn);
    }
    ste2007_chipselect(dpyH, 1);

    SemaphoreP_post(o->mutex);
}

//! @brief Erase a single line - TI Display_clearLines() handler
void ste2007_clearLines(Display_Handle dpyH, uint8_t start, uint8_t end)
{
    int i;
    DisplayNokia1202_Object *o = dpyH->object;

    SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);

    for(i=0; i < 16*6; i++) {
        o->_rowBuf[i] = 0x0100;
    }
    o->rowbuffer.len = 16*6;

    SPI_Transaction txn;
    txn.count = o->rowbuffer.len;
    txn.txBuf = (void *)(o->rowbuffer.buf);
    txn.rxBuf = (void *)0;

    if (end < start) {
        // Somewhat undefined behavior in the docs, but, the DisplaySharp library uses this logic.
        // The Display_clearLine() macro depends on using Display_doClearLines(handle, start, 0) to erase a single line.
        end = start;
    }

    for (i=start; i <= end; i++) {
        ste2007_setxy(dpyH, 0, i);
        ste2007_chipselect(dpyH, 0);
        SPI_transfer(o->bus, &txn);
        ste2007_chipselect(dpyH, 1);
    }

    SemaphoreP_post(o->mutex);
}


//! @brief Set DDRAM cursor
void ste2007_setxy(Display_Handle dpyH, uint8_t x, uint8_t y)
{
    ste2007_issuecmd(dpyH, STE2007_CMD_LINE, y, STE2007_MASK_LINE);
    ste2007_issuecmd(dpyH, STE2007_CMD_COLMSB, x >> 4, STE2007_MASK_COLMSB);
    ste2007_issuecmd(dpyH, STE2007_CMD_COLLSB, x, STE2007_MASK_COLLSB);
}

//! @brief Bulk-write data to DDRAM
//! @details Note: This function does not drive the Chip Select line but assumes that you will before/after running this.
void ste2007_write(Display_Handle dpyH, const void *buf, uint16_t len)
{
    uint32_t ttl = 0;
    uint8_t *ubuf = (uint8_t *)buf;
    DisplayNokia1202_Object *o = dpyH->object;

    while (ttl < len) {
        spitxn_reset(&(o->rowbuffer));
        spitxn_push(&(o->rowbuffer), 0x01, ubuf+ttl, ((len - ttl) > o->rowbuffer.cap ? o->rowbuffer.cap : len-ttl));

        SPI_Transaction txn;
        txn.count = o->rowbuffer.len;
        txn.txBuf = (void *)(o->rowbuffer.buf);
        txn.rxBuf = (void *)0;

        SPI_transfer(o->bus, &txn);

        ttl += o->rowbuffer.len;
    }
}

//! @brief Set/unset the DisplayReverse feature
void ste2007_invert(Display_Handle dpyH, uint8_t onoff)
{
    ste2007_issuecmd(dpyH, STE2007_CMD_DPYREV, onoff, STE2007_MASK_DPYREV);
}

//! @brief STE2007 datasheet lists ONOFF=0, DPYALLPTS=1 as a "Power saver" mode.
void ste2007_powersave(Display_Handle dpyH, uint8_t onoff)  // 1 = power-saver mode, 0 = normal mode
{
    ste2007_issuecmd(dpyH, STE2007_CMD_DPYALLPTS, onoff, STE2007_MASK_DPYALLPTS);
    ste2007_issuecmd(dpyH, STE2007_CMD_ONOFF, !onoff, STE2007_MASK_ONOFF);
}

/**
 * @brief Set contrast
 * @details val is a scale from 0-31 and configures the Electronic Volume setting.
 *          16 is the default.
 */
void ste2007_contrast(Display_Handle dpyH, uint8_t val)
{
    ste2007_issuecmd(dpyH, STE2007_CMD_ELECTVOL, val, STE2007_MASK_ELECTVOL);
}

//! @brief Set LCD refresh rate
//! @details Supported values: 65, 70, 75, 80 (Hz)
void ste2007_refreshrate(Display_Handle dpyH, uint8_t val)
{
    switch (val) {
        case 80:
            ste2007_issue_compoundcmd(dpyH, STE2007_CMD_REFRESHRATE, 0, STE2007_MASK_REFRESHRATE);
            break;

        case 75:
            ste2007_issue_compoundcmd(dpyH, STE2007_CMD_REFRESHRATE, 1, STE2007_MASK_REFRESHRATE);
            break;

        case 70:
            ste2007_issue_compoundcmd(dpyH, STE2007_CMD_REFRESHRATE, 2, STE2007_MASK_REFRESHRATE);
            break;

        default:
            ste2007_issue_compoundcmd(dpyH, STE2007_CMD_REFRESHRATE, 3, STE2007_MASK_REFRESHRATE);
    }
}


//! @brief vprintf for TI Display printf API
void ste2007_vprintf(Display_Handle dpyH, uint8_t line, uint8_t col, char *fmt, va_list va)
{
    int i;
    char dispStr[32], *c;
    DisplayNokia1202_Object *o = dpyH->object;

    SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);

    // Act upon lineClearMode first (so we can use dispStr for an alternate purpose before applying vsnprintf)
    if (o->lineClearMode != DISPLAY_CLEAR_NONE) {
        if (o->lineClearMode == DISPLAY_CLEAR_LEFT) {
            for(i=0; i < col; i++) {
                dispStr[i] = '\0';
            }
            ste2007_setxy(dpyH, 0, line);
            ste2007_chipselect(dpyH, 0);
            ste2007_write(dpyH, dispStr, col);
            ste2007_chipselect(dpyH, 1);
        } else if (o->lineClearMode == DISPLAY_CLEAR_RIGHT) {
            for (i=0; i < (16-col); i++) {
                dispStr[i] = '\0';
            }
            ste2007_setxy(dpyH, col, line);
            ste2007_chipselect(dpyH, 0);
            ste2007_write(dpyH, dispStr, 16-col);
            ste2007_chipselect(dpyH, 1);
        } else if (o->lineClearMode == DISPLAY_CLEAR_BOTH) {
            // Need to release the mutex because ste2007_clearLines() uses it
            SemaphoreP_post(o->mutex);
            ste2007_clearLines(dpyH, line, line);
            SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);
        }
    }

    SystemP_vsnprintf(dispStr, sizeof(dispStr), fmt, va);  // SimpleLink SDK provides a worker function for the hard part here in the DPL (Driver Porting Layer)...

    // Write out dispStr
    ste2007_setxy(dpyH, col, line);
    c = &dispStr[0];

    ste2007_chipselect(dpyH, 0);
    while (*c) {
        ste2007_write(dpyH, font_5x7[(unsigned int)*c - 32], 6);
        c++;
    }
    ste2007_chipselect(dpyH, 1);

    SemaphoreP_post(o->mutex);
}


//! @brief Boilerplate driver function - return display type
unsigned int ste2007_getType()
{
    return Display_Type_LCD;
}


//! @brief Wrapper to avoid littering ste2007_control() with Semaphore_post's at every return
int ste2007_control_mutexwrapped(Display_Handle dpyH, unsigned int cmd, void *arg)
{
    DisplayNokia1202_Object *o = dpyH->object;
    int ret;

    SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);
    ret = ste2007_control(dpyH, cmd, arg);
    SemaphoreP_post(o->mutex);

    return ret;
}

//! @brief Custom control of the display - Display_control() handler
//! @details A list of custom command codes are found in ste2007.h with possible non-standard return values as well.
//! @return Signed integer equal to DISPLAY_STATUS_SUCCESS, DISPLAY_STATUS_ERROR or a custom return value
int ste2007_control(Display_Handle dpyH, unsigned int cmd, void *arg)
{
    uint8_t *u8ptr;
    const DisplayNokia1202_HWAttrsV1 *h = dpyH->hwAttrs;

    /* Note: The display's mutex is in a pended state when this function runs, so if we need to run
     * any mutex-dependent functions e.g. ste2007_clear(), we should grab the dpyH->object->mutex and post, followed
     * by a pend after the mutex-dependent function completes.
     */

    switch (cmd) {
        case NOKIA1202_CMD_CONTRAST:
            if (arg == (void *)0) {
                return DISPLAY_STATUS_ERROR;
            }
            u8ptr = (uint8_t *)arg;
            if (*u8ptr > 31) {
                return NOKIA1202_CONTRAST_OUT_OF_RANGE;
            }
            ste2007_contrast(dpyH, *u8ptr);
            return DISPLAY_STATUS_SUCCESS;

        case NOKIA1202_CMD_REFRESHRATE:
            if (arg == (void *)0) {
                return DISPLAY_STATUS_ERROR;
            }
            u8ptr = (uint8_t *)arg;
            if (*u8ptr != 65 && *u8ptr != 70 && *u8ptr != 75 && *u8ptr != 80) {
                return NOKIA1202_REFRESHRATE_INVALID;
            }
            ste2007_refreshrate(dpyH, *u8ptr);
            return DISPLAY_STATUS_SUCCESS;

        case NOKIA1202_CMD_INVERT:
            if (arg == (void *)0) {
                return DISPLAY_STATUS_ERROR;
            }
            u8ptr = (uint8_t *)arg;

            if (*u8ptr) {
                ste2007_invert(dpyH, 1);
            } else {
                ste2007_invert(dpyH, 0);
            }
            return DISPLAY_STATUS_SUCCESS;

        case NOKIA1202_CMD_POWERSAVE:
            if (arg == (void *)0) {
                return DISPLAY_STATUS_ERROR;
            }
            u8ptr = (uint8_t *)arg;

            if (*u8ptr) {
                ste2007_powersave(dpyH, 1);
            } else {
                ste2007_powersave(dpyH, 0);
            }
            return DISPLAY_STATUS_SUCCESS;

        case NOKIA1202_CMD_BACKLIGHT:
            if (arg == (void *)0) {
                return DISPLAY_STATUS_ERROR;
            }
            u8ptr = (uint8_t *)arg;

            if (!h->useBacklight) {
                return DISPLAY_STATUS_SUCCESS;  // Nothing to do here so just pretend all's good
            }
            if (*u8ptr) {
                GPIO_write(h->backlightPin, 1);
            } else {
                GPIO_write(h->backlightPin, 0);
            }
            return DISPLAY_STATUS_SUCCESS;
    }

    return DISPLAY_STATUS_UNDEFINEDCMD;  // Command not found
}

//! @brief TI Display_close() handler
void ste2007_close(Display_Handle dpyH)
{
    DisplayNokia1202_Object *o = dpyH->object;

    SemaphoreP_pend(o->mutex, SemaphoreP_WAIT_FOREVER);

    ste2007_chipselect(dpyH, 1);
    SPI_close(o->bus);
    o->bus = NULL;

    SemaphoreP_post(o->mutex);
}
