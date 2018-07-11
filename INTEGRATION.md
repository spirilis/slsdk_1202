# Integrating nokia1202 into SimpleLink SDK projects
The nokia1202 library can be checked out somewhere on your computer and the `nokia1202` folder "linked back" into your CCS project.

Example:

Import "display" example for MSP432E4:

![CCSv8 importing the MSP432E4 Display example](https://raw.githubusercontent.com/spirilis/slsdk_1202/master/docs/ccsv8_msp432e4_display_example.png)

Import the `nokia1202` folder into the project (Right-click > Import > Import...) then (General > File System) and Next:

![CCSv8 linking nokia1202 folder into project](https://raw.githubusercontent.com/spirilis/slsdk_1202/master/docs/ccsv8_import_nokia1202.png)

CCSv8.1 has a very nice feature when you link code folders into your project - it will ask if you want to auto-add the folder to the compiler's Include search path.  Say yes.

![CCSv8 asking if you want to auto-add the linked path to your include search path](https://raw.githubusercontent.com/spirilis/slsdk_1202/master/docs/ccsv8_auto_adjust_includepath.png)

If you're running a version of CCS that doesn't do this, please add it using (Right-click > Properties) then (Build > ARM Compiler > Include Options) and add the path to `nokia1202` to the "Add dir to #include search path (--include-path, -I)" section.

## Using the nokia1202 library

The library is designed to integrate into the TI Display API within the TI-Drivers framework.

To illustrate how this works, open the board-specific TI-Drivers config .c file - MSP_EXP432E401Y.c in this case - and scroll to the "Display" section:
![MSP_EXP432E401Y.c display section](https://raw.githubusercontent.com/spirilis/slsdk_1202/master/docs/msp432e4_board_dot_c_display_section.png)

The stock `Display_config[]` for this lists the UART display and Sharp96 Memory LCD display.  Since we open displays in this example by "type", e.g. `Display_open(Display_Type_LCD, NULL)`
we will want to remove the Sharp96 entry (since our Nokia 1202 registers itself as an LCD just like the Sharp):

```c
/*
 * This #if/#else is needed to workaround a problem with the
 * IAR compiler. The IAR compiler doesn't like the empty array
 * initialization. (IAR Error[Pe1345])
 */
 #if (BOARD_DISPLAY_USE_UART || BOARD_DISPLAY_USE_LCD)
const Display_Config Display_config[] = {
    {
#  if (BOARD_DISPLAY_USE_UART_ANSI)
        .fxnTablePtr = &DisplayUartAnsi_fxnTable,
#  else /* Default to minimal UART with no cursor placement */
        .fxnTablePtr = &DisplayUartMin_fxnTable,
#  endif
        .object = &displayUartObject,
        .hwAttrs = &displayUartHWAttrs
    },
#endif
// COMMENT THIS PART OUT-
/* #if (BOARD_DISPLAY_USE_LCD)
    {
        .fxnTablePtr = &DisplaySharp_fxnTable,
        .object      = &displaySharpObject,
        .hwAttrs     = &displaySharpHWattrs
    },
#endif */
};
```

Before we insert our nokia1202 driver entry into the `Display_config[]` array, we need to predefine the HWAttrs struct and Object struct which it uses for configuration & buffers/state.

You'll see above this section where structs are defined for the Sharp96 and UART:

```c
/*
 *  ============================= Display =============================
 */
#include <ti/display/Display.h>
#include <ti/display/DisplayUart.h>
#include <ti/display/DisplaySharp.h>
#define MAXPRINTLEN 1024

#ifndef BOARD_DISPLAY_SHARP_SIZE
#define BOARD_DISPLAY_SHARP_SIZE    96
#endif

DisplayUart_Object displayUartObject;
DisplaySharp_Object    displaySharpObject;

static char displayBuf[MAXPRINTLEN];
static uint_least8_t sharpDisplayBuf[BOARD_DISPLAY_SHARP_SIZE * BOARD_DISPLAY_SHARP_SIZE / 8];


const DisplayUart_HWAttrs displayUartHWAttrs = {
    .uartIdx = MSP_EXP432E401Y_UART0,
    .baudRate = 115200,
    .mutexTimeout = (unsigned int)(-1),
    .strBuf = displayBuf,
    .strBufLen = MAXPRINTLEN
};

const DisplaySharp_HWAttrsV1 displaySharpHWattrs = {
    .spiIndex    = MSP_EXP432E401Y_SPI2,
    .csPin       = MSP_EXP432E401Y_LCD_CS,
    .powerPin    = MSP_EXP432E401Y_LCD_POWER,
    .enablePin   = MSP_EXP432E401Y_LCD_ENABLE,
    .pixelWidth  = BOARD_DISPLAY_SHARP_SIZE,
    .pixelHeight = BOARD_DISPLAY_SHARP_SIZE,
    .displayBuf  = sharpDisplayBuf,
};
```

If you'd like, you can remove any of the `DisplaySharp` related stuff but it's not strictly necessary since those structs should be garbage-collected by the compiler since we have no references to the Sharp display in `Display_config[]`.

However, we want to add our Nokia1202 config after that code:

```c
#include <ste2007.h>
const DisplayNokia1202_HWAttrsV1 nokiaConfig = {
    .spiBus = MSP_EXP432E401Y_SPI2,
    .csPin = NOKIA1202_GPIO_CS,
    .backlightPin = NOKIA1202_GPIO_BACKLIGHT_LED,
    .useBacklight = true
};

DisplayNokia1202_Object nokiaBuffers;
```

Note the references to `MSP_EXP432E401Y_SPI2`, `NOKIA1202_GPIO_CS` and `NOKIA1202_GPIO_BACKLIGHT_LED`.  These refer to other TI-Drivers config arrays in the MSP_EXP432E401Y.c and MSP_EXP432E401Y.h files (hereto referred to as "`board.c` and `board.h`"), and advanced discussion of how to configure these items is outside the scope of this document - please consult the SimpleLink Academy to learn how you add GPIOs to the `gpioPinConfigs[]` array, add entries to the board.h for the GPIOName enum (which positionally refers to members of the `gpioPinConfigs[]` array), and if the necessary SPI configuration isn't already there (on the pins where you need them) you will have to modify the SPI stuff.  That said, I'll show you my example of how I do it.

For the MSP432E401Y SimpleLink Wired Ethernet microcontroller, we are using SSI2 (SPI bus#2), PC7 (port C, pin#7) for the SPI Chip Select pin and PN2 (port N, pin#2) for the backlight LED.

Our stock `spiMSP432E4DMAHWAttrs` array is fine as the first entry refers to SSI2 on the correct pins, so we can use the first entry in the `MSP_EXP432E401Y_SPIName` enum (from `board.h`):

```c
/*!
 *  @def    MSP_EXP432E401Y_SPIName
 *  @brief  Enum of SPI names on the MSP_EXP432E401Y dev board
 */
typedef enum MSP_EXP432E401Y_SPIName {
    MSP_EXP432E401Y_SPI2 = 0,
    MSP_EXP432E401Y_SPI3,

    MSP_EXP432E401Y_SPICOUNT
} MSP_EXP432E401Y_SPIName;
```

For the CS and Backlight pins, we have to add configuration to the end of `gpioPinConfigs[]` to accommodate them:

```c
GPIO_PinConfig gpioPinConfigs[] = {
    /* Input pins */
    /* MSP_EXP432E401Y_USR_SW1 */
    GPIOMSP432E4_PJ0 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,
    /* MSP_EXP432E401Y_USR_SW2 */
    GPIOMSP432E4_PJ1 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_RISING,

    /* MSP_EXP432E401Y_SPI_MASTER_READY */
    GPIOMSP432E4_PM3 | GPIO_DO_NOT_CONFIG,
    /* MSP_EXP432E401Y_SPI_SLAVE_READY */
    GPIOMSP432E4_PL0 | GPIO_DO_NOT_CONFIG,

    /* Output pins */
    /* MSP_EXP432E401Y_USR_D1 */
    GPIOMSP432E4_PN1 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
    /* MSP_EXP432E401Y_USR_D2 */
    GPIOMSP432E4_PN0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,

    /* MSP_EXP432E401Y_SDSPI_CS */
    GPIOMSP432E4_PC7 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_HIGH,

    /* Sharp Display - GPIO configurations will be done in the Display files */
    GPIOMSP432E4_PE5 | GPIO_DO_NOT_CONFIG, /* SPI chip select */
    GPIOMSP432E4_PC6 | GPIO_DO_NOT_CONFIG, /* LCD power control */
    GPIOMSP432E4_PE4 | GPIO_DO_NOT_CONFIG, /*LCD enable */

    /* Nokia 1202 pins */
    GPIOMSP432E4_PC7 | GPIO_DO_NOT_CONFIG, /* Nokia 1202 chip select */
    GPIOMSP432E4_PN2 | GPIO_DO_NOT_CONFIG, /* Nokia 1202 backlight LED */
};
```

For our `NOKIA1202_GPIO_`* references, we add these to the board.h file:

```c
/*!
 *  @def    MSP_EXP432E401Y_GPIOName
 *  @brief  Enum of LED names on the MSP_EXP432E401Y dev board
 */
typedef enum MSP_EXP432E401Y_GPIOName {
    MSP_EXP432E401Y_GPIO_USR_SW1 = 0,
    MSP_EXP432E401Y_GPIO_USR_SW2,
    MSP_EXP432E401Y_SPI_MASTER_READY,
    MSP_EXP432E401Y_SPI_SLAVE_READY,
    MSP_EXP432E401Y_GPIO_D1,
    MSP_EXP432E401Y_GPIO_D2,

    MSP_EXP432E401Y_SDSPI_CS,

    /* Sharp 96x96 LCD Pins */
    MSP_EXP432E401Y_LCD_CS,
    MSP_EXP432E401Y_LCD_POWER,
    MSP_EXP432E401Y_LCD_ENABLE,

    /* Nokia 1202 pins */
    NOKIA1202_GPIO_CS,               // <- This
    NOKIA1202_GPIO_BACKLIGHT_LED,    // <- And This

    MSP_EXP432E401Y_GPIOCOUNT
} MSP_EXP432E401Y_GPIOName;
```

Now that we have our GPIOs identified and SPI bus identified in our HWAttrs struct, let's add these to `Display_config[]` to finalize everything:

```c
/*
 * This #if/#else is needed to workaround a problem with the
 * IAR compiler. The IAR compiler doesn't like the empty array
 * initialization. (IAR Error[Pe1345])
 */
 #if (BOARD_DISPLAY_USE_UART || BOARD_DISPLAY_USE_LCD)
const Display_Config Display_config[] = {
    {
#  if (BOARD_DISPLAY_USE_UART_ANSI)
        .fxnTablePtr = &DisplayUartAnsi_fxnTable,
#  else /* Default to minimal UART with no cursor placement */
        .fxnTablePtr = &DisplayUartMin_fxnTable,
#  endif
        .object = &displayUartObject,
        .hwAttrs = &displayUartHWAttrs
    },
#endif
// COMMENT THIS PART OUT-
/* #if (BOARD_DISPLAY_USE_LCD)
    {
        .fxnTablePtr = &DisplaySharp_fxnTable,
        .object      = &displaySharpObject,
        .hwAttrs     = &displaySharpHWattrs
    },
#endif */
    /* Nokia 1202 entry */
    {
        .fxnTablePtr = &DisplayNokia1202_FxnTable,  // Found in ste2007.h/.c
        .object = &nokiaBuffers,
        .hwAttrs = &nokiaConfig
    },
```

At this point, you should be able to build the firmware and the stock "display" example will open the Nokia 1202 display and start printing text.

Default values are used for contrast, refresh rate, etc. but those should be reasonable.  If they are not, you will have to utilize the `Display_control()` feature in your firmware to adjust them.  `#include <ste2007.h>` first so you have the command macros for the various tunables that you can access via `Display_control()`.

See the end of [ste2007.h](https://github.com/spirilis/slsdk_1202/blob/master/nokia1202/ste2007.h) for info on these tunables.  Use them with `Display_control()` e.g.:

```c
    /* Check if the selected Display type was found and successfully opened */
    if (hLcd) {
        Display_printf(hLcd, 5, 3, "Hello LCD!");

        uint8_t c = 8;
        Display_control(hLcd, NOKIA1202_CMD_CONTRAST, &c);  // Set contrast level to 8 (out of 0-31)
        c = 1;
        Display_control(hLcd, NOKIA1202_CMD_BACKLIGHT, &c); // Switch backlight LED on (since c=1)

        /* Wait a while so text can be viewed. */
        sleep(3);
```