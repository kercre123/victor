/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdlib.h>
#include "gpio_pins.h"
#include "device/fsl_device_registers.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Declare Switch pins with EnableIRQ*/
gpio_input_pin_user_config_t switchPinsIrq[] = {
    {
        .pinName = kGpioSW1,
        .config.isPullEnable = true,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = true,
        .config.interrupt = kPortIntFallingEdge,
    },
    {
        .pinName = kGpioSW2,
        .config.isPullEnable = true,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = true,
        .config.interrupt = kPortIntFallingEdge,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};
/* Declare Switch pins */
gpio_input_pin_user_config_t switchPins[] = {
    {
        .pinName = kGpioSW1,   
        .config.isPullEnable = false,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntDisabled,
    },
    {
        .pinName = kGpioSW2,
        .config.isPullEnable = false,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntDisabled,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};

gpio_input_pin_user_config_t accelIntPins[] = {
    {
        .pinName = kGpioAccelINT1,
        .config.isPullEnable = false,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntDisabled,
    },
    {
        .pinName = kGpioAccelINT2,
        .config.isPullEnable = false,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntDisabled,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};


/* Declare Input pins for I2C Address Variables in I2C demo */
gpio_input_pin_user_config_t i2cAddressPins[] = {
    {
        .pinName = kGpioI2Caddr1,
        .config.isPullEnable = true,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntDisabled,
    },
    {
        .pinName = kGpioI2Caddr2,
        .config.isPullEnable = true,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntDisabled,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};

/* Declare UART 1 TX / RX pins for the gpio_uart demo */
gpio_output_pin_user_config_t gpioUartDemoTxPin[] = {
    {
        .pinName = kGpioUartDemoTX,
        .config.outputLogic = 1,
        .config.slewRate = kPortSlowSlewRate,
        .config.driveStrength = kPortLowDriveStrength,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};

/* Declare UART 1 TX / RX pins for the gpio_uart demo */
gpio_input_pin_user_config_t gpioUartDemoRxPin[] = {
    {
        .pinName = kGpioUartDemoRX,
        .config.isPullEnable = false,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntFallingEdge,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};

/* Declare Output GPIO pins */
gpio_output_pin_user_config_t ledPins[] = {
    {
        .pinName = kGpioLED1,
        .config.outputLogic = 1,
        .config.slewRate = kPortSlowSlewRate,
        .config.driveStrength = kPortLowDriveStrength,
    },
    {
        .pinName = kGpioLED2,
        .config.outputLogic = 1,
        .config.slewRate = kPortSlowSlewRate,
        .config.driveStrength = kPortLowDriveStrength,
    },
    {
        .pinName = kGpioLED3,
        .config.outputLogic = 1,
        .config.slewRate = kPortSlowSlewRate,
        .config.driveStrength = kPortLowDriveStrength,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};

/* Declare SPI0 CS0 pins for the spi flash chip select */
gpio_output_pin_user_config_t spiCsPin[] = {
    {
        .pinName = kGpioSpi0Cs0,
        .config.outputLogic = 1,
        .config.slewRate = kPortSlowSlewRate,
        .config.driveStrength = kPortLowDriveStrength,
    },
        {
        .pinName = kGpioSpi0Cs1,
        .config.outputLogic = 1,
        .config.slewRate = kPortSlowSlewRate,
        .config.driveStrength = kPortLowDriveStrength,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};

/* Declare RF chip enable pin */
gpio_output_pin_user_config_t rfCEPin[] = {
    {
        .pinName = kGpioRFCE,
        .config.outputLogic = 1,
        .config.slewRate = kPortSlowSlewRate,
        .config.driveStrength = kPortLowDriveStrength,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};

/* Declare RF Chip IRQ pin */
gpio_input_pin_user_config_t rfIRQPin[] = {
    {
        .pinName = kGpioRFIRQ,
        .config.isPullEnable = false,
        .config.pullSelect = kPortPullUp,
        .config.isPassiveFilterEnabled = false,
        .config.interrupt = kPortIntFallingEdge,
    },
    {
        .pinName = GPIO_PINS_OUT_OF_RANGE,
    }
};
