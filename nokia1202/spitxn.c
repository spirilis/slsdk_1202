/**
 * @file spitxn.c
 * @brief SPI Transaction buffer helper
 * @details This code allows one to stuff an arbitrary buffer of 16-bit values with contents from an 8-bit array
 *          for the purpose of adapting 8-bit data to 16-bit SPI, with a possible static bit set in the upper 8 bits
 *          to account for special situations e.g. 9-bit SPI where the 9th bit is used as a Data/Control signal with
 *          certain LCD displays.
 *
 * @author Eric Brundick
 * @date 2018
 * @version 100
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
#include "spitxn.h"

//! @brief Reset and fully erase a buffer, assigning all values from index 0 to <cap> to 0x0000.
void spitxn_erase(SpiTxn_buffer *buf)
{
    uint32_t i = 0;

    if (buf->cap > 0) {
        for (i=0; i < buf->cap; i++) {
            buf->buf[i] = (uint16_t)0;
        }
    }
    buf->len = 0;
}

//! @brief Reset a buffer by setting <len> to 0, without physically changing the underlying buffer values.
void spitxn_reset(SpiTxn_buffer *buf)
{
    buf->len = 0;
}

/**
 * @brief Append 8-bit data to a buffer, with an optional high-byte OR'd to every 16-bit word.
 * @details This function takes a buffer and begins walking through your <data> array for <len> bytes, appending each
 *          byte to the end of the buffer as defined by the buffer's own <len> member.  Multiple executions of this
 *          function without a reset or erase operation will continually grow the dataset in the buffer up to <cap>
 *          words.
 *          Supplying a value for highTag that is not 0 will left-shift this value by 8 bits and OR it to every byte
 *          written to the buffer.  For 9-bit SPI LCD data, this should be set to 1 if the 9th bit is used to signify DDRAM data
 *          for an LCD framebuffer.
 * @return The exact # of bytes read from <data> and packed into the buffer, which may be less than <len> if the buffer
 *         filled up before completion.
 */
uint32_t spitxn_push(SpiTxn_buffer *buf, uint8_t highTag, uint8_t *data, uint32_t len)
{
    uint32_t i = 0, c = 0;

    if (buf->cap > 0) {
        i = buf->len;
        while ((buf->cap - i) > 0 && (c < len)) {
            buf->buf[i] = (uint16_t)(data[c]) | ((uint16_t)highTag << 8);
            c++;
            i++;
            buf->len++;
        }
        return c;
    } else {
        return 0;
    }
}

/**
 * @brief Erase the last <len> bytes from the current buffer, erasing the underlying buffer contents along the way.
 * @return The number of bytes erased, which may be fewer than <len> if the buffer contained less than <len> bytes.
 */
uint32_t spitxn_pop(SpiTxn_buffer *buf, uint32_t len)
{
    int32_t i = 0;
    uint32_t c = 0;

    if (buf->cap > 0) {
        i = buf->len-1;
        while ((i > -1) && (c < len)) {
            buf->buf[i] = (uint16_t)0;
            buf->len--;
            i--;
            c++;
        }
        return c;
    } else {
        return 0;
    }
}
