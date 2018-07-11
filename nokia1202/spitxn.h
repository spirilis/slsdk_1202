/**
 * @file spitxn.h
 * @brief SPI Transaction buffer helper - header
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

#ifndef NOKIA1202_SPITXN_H_
#define NOKIA1202_SPITXN_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief The buffer's state.
 * @details Initializing an SpiTxn_buffer can be done by hand; the user can declare an SpiTxn_buffer object
 *          and manually set the buf and cap variables to reflect the amount of storage space available.
 *          Note that all spitxn_* functions require a pointer, so an in-place declared buffer will be referenced
 *          with a prepended & e.g. spitxn_erase(&spibuf)
 */
typedef struct {
    uint32_t len;
    uint32_t cap;
    uint16_t * buf;
} SpiTxn_buffer;

void spitxn_erase(SpiTxn_buffer * buf); //! @brief Erase the entire buffer from 0 to <cap> and reset <len> to 0.
void spitxn_reset(SpiTxn_buffer * buf); //! @brief Quick-erase buffer by resetting <len> to position 0 without erasing data.
uint32_t spitxn_push(SpiTxn_buffer * buf, uint8_t highTag, uint8_t * data, uint32_t len); //! @brief Adds <len> bytes from <data> to the end of the buffer, converting to uint16_t words, OR'ing each word with (highTag << 8).
uint32_t spitxn_pop(SpiTxn_buffer * buf, uint32_t len);  //! @brief Erases last <len> words from the buffer


#endif /* NOKIA1202_SPITXN_H_ */
