/**
 * @file ste2007.h
 * @brief Nokia 1202 STE2007 TI Display Driver - Main header
 * @author Eric Brundick
 * @date 2018
 * @version 100
 *
 * @details Nokia 1202 TI Display driver API - Include this header file inside your <board>.c as well as client application firmware files
 *          for those functions that require the NOKIA1202_CMD_* and related Display_control() macros.
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

#ifndef NOKIA1202_STE2007_H_
#define NOKIA1202_STE2007_H_


#include <stdint.h>
#include <ti/display/Display.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include "spitxn.h"

/* These commands are standard CMD | DATA operations.
 * The byte sent is CMD_* OR'd by (DATA & MASK_*) with
 *   the 9th bit set to 0 indicating Command.
 */

#define STE2007_CMD_ONOFF 0xAE
#define STE2007_MASK_ONOFF 0x01

#define STE2007_CMD_DPYREV 0xA6
#define STE2007_MASK_DPYREV 0x01

#define STE2007_CMD_DPYALLPTS 0xA4
#define STE2007_MASK_DPYALLPTS 0x01

#define STE2007_CMD_LINE 0xB0
#define STE2007_MASK_LINE 0x0F

#define STE2007_CMD_COLMSB 0x10
#define STE2007_MASK_COLMSB 0x07

#define STE2007_CMD_COLLSB 0x00
#define STE2007_MASK_COLLSB 0x0F

#define STE2007_CMD_DPYSTARTLINE 0x40
#define STE2007_MASK_DPYSTARTLINE 0x3F

#define STE2007_CMD_SEGMENTDIR 0xA0
#define STE2007_MASK_SEGMENTDIR 0x01

#define STE2007_CMD_COMDIR 0xC0
#define STE2007_MASK_COMDIR 0x08

#define STE2007_CMD_PWRCTL 0x28
#define STE2007_MASK_PWRCTL 0x07

#define STE2007_CMD_VORANGE 0x40
#define STE2007_MASK_VORANGE 0x07

#define STE2007_CMD_ELECTVOL 0x80
#define STE2007_MASK_ELECTVOL 0x1F

#define STE2007_CMD_RESET 0xE2
#define STE2007_MASK_RESET 0x00

#define STE2007_CMD_NOP 0xE3
#define STE2007_MASK_NOP 0x00

// VOP is set by submitting CMD_VOP, then the value as the next 9-bit byte.
#define STE2007_CMD_VOP 0xE1
#define STE2007_MASK_VOP 0xFF

// VLCD Slope is similar; submit CMD_VLCDSLOPE, then the value in the next 9-bit byte.
#define STE2007_CMD_VLCDSLOPE 0x38
#define STE2007_MASK_VLCDSLOPE 0x07

// Charge Pump similar.
#define STE2007_CMD_CHARGEPUMP 0x3D
#define STE2007_MASK_CHARGEPUMP 0x03

// Refresh Rate similar.
#define STE2007_CMD_REFRESHRATE 0xEF
#define STE2007_MASK_REFRESHRATE 0x03

// Bias ratio is a normal CMD|DATA operation.
#define STE2007_CMD_SETBIAS 0x30
#define STE2007_MASK_SETBIAS 0x07

// N-Line inversion is a compound command (send CMD_NLINEINV, then value)
#define STE2007_CMD_NLINEINV 0xAD
#define STE2007_MASK_NLINEINV 0x1F

// Number of Lines is a normal CMD|DATA operation.
#define STE2007_CMD_NUMLINES 0xD0
#define STE2007_MASK_NUMLINES 0x07

// Image Location is a compound command (send CMD_IMAGELOC, then value)
#define STE2007_CMD_IMAGELOC 0xAC
#define STE2007_MASK_IMAGELOC 0x07

// Icon Mode is a normal CMD|DATA operation.
#define STE2007_CMD_ICONMODE 0xF8
#define STE2007_MASK_ICONMODE 0x01


/**
 * @brief Library functions for the STE2007 driver
 * @details These functions are used internally by the TI Display_* API, and live in the FxnTable used by
 *          the Display API.
 */

void ste2007_chipselect(Display_Handle, uint8_t onoff);
void ste2007_init(Display_Handle);  // just initializes the object members
Display_Handle ste2007_open(Display_Handle, Display_Params *);  // opens SPI bus and initializes the chip
void ste2007_issuecmd(Display_Handle, uint8_t cmd, uint8_t arg, uint8_t argmask);
void ste2007_issue_compoundcmd(Display_Handle, uint8_t cmd, uint8_t arg, uint8_t argmask);
void ste2007_clear(Display_Handle);
void ste2007_clearLines(Display_Handle, uint8_t start, uint8_t end);
void ste2007_setxy(Display_Handle, uint8_t x, uint8_t y);
void ste2007_write(Display_Handle, const void *buf, uint16_t len);
void ste2007_invert(Display_Handle, uint8_t onoff);
void ste2007_powersave(Display_Handle, uint8_t onoff);
void ste2007_contrast(Display_Handle, uint8_t val);
void ste2007_refreshrate(Display_Handle, uint8_t val);


/* TI-RTOS struct definitions */

//! @brief HWAttrs struct definition for static runtime config of the display
typedef struct {
    uint32_t spiBus;
    uint32_t csPin;
    uint32_t backlightPin;
    bool useBacklight;
} DisplayNokia1202_HWAttrsV1;

/**
 * @brief Object struct definition holds the buffers and state; this should never be initialized by the user
 * @details The Nokia1202 driver is thread-safe using a semaphore as mutex but note individual operations directly
 *          write to the display; written data is not queued in a Mailbox first and handled by a secondary task.
 */
typedef struct {
    SpiTxn_buffer cmdBuf;
    uint16_t _cmdBuffer[4];
    SpiTxn_buffer rowbuffer;
    uint16_t _rowBuf[16*6]; // Stores up to 1 row worth of data
    SPI_Handle bus;
    Display_LineClearMode lineClearMode;
    SemaphoreP_Handle mutex;
} DisplayNokia1202_Object;

//! @brief Function table - this needs to be stuffed into your Display_config[] array for your <board>.c file
extern const Display_FxnTable DisplayNokia1202_FxnTable;

/* User-facing control commands */

//! @brief Display_control() command to adjust display contrast
//! @details CMD_CONTRAST takes a uint8_t argument from 0-31
#define NOKIA1202_CMD_CONTRAST              (DISPLAY_CMD_RESERVED + 0)
#define NOKIA1202_CONTRAST_OUT_OF_RANGE     (DISPLAY_STATUS_RESERVED - 0)

//! @brief Display_control() command to adjust display refresh rate
//! @details CMD_REFRESHRATE takes a uint8_t argument of 65, 70, 75, 80
#define NOKIA1202_CMD_REFRESHRATE           (DISPLAY_CMD_RESERVED + 1)
#define NOKIA1202_REFRESHRATE_INVALID       (DISPLAY_STATUS_RESERVED - 1)

//! @brief Display_control() command to invert the display's pixels
//! @details CMD_INVERT takes a uint8_t argument of 0, !0
#define NOKIA1202_CMD_INVERT                (DISPLAY_CMD_RESERVED + 2)

//! @brief Display_control() command to bring the display in or out of Powersave mode
//! @details CMD_POWERSAVE takes a uint8_t of 0 (poweron), !0 (poweroff)
#define NOKIA1202_CMD_POWERSAVE             (DISPLAY_CMD_RESERVED + 3)

//! @brief Display_control() command to control the display's backlight LED
//! @details CMD_BACKLIGHT takes a uint8_t of 0 (off), !0 (on)
#define NOKIA1202_CMD_BACKLIGHT             (DISPLAY_CMD_RESERVED + 4)


#endif /* NOKIA1202_STE2007_H_ */
