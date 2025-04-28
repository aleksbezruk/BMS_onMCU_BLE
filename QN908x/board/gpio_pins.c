/*! *********************************************************************************
* Copyright 2013-2015, Freescale Semiconductor, Inc.
* Copyright 2016-2018 NXP
* All rights reserved.
*
* \file
*
* SPDX-License-Identifier: BSD-3-Clause
********************************************************************************** */

#include <stdint.h>
#include <stdlib.h>
#include "GPIO_Adapter.h"
#include "gpio_pins.h"
#include "board.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

gpioInputPinConfig_t switchPins[] = {
    {
        .gpioPort = gpioPort_A_c,
        .gpioPin = BOARD_SW1_GPIO_PIN,
        .pullSelect = pinPull_Up_c,
        .interruptSelect = pinInt_FallingEdge_c
    },
    {
        .gpioPort = gpioPort_A_c,
        .gpioPin = BOARD_SW2_GPIO_PIN,
        .pullSelect = pinPull_Up_c,
        .interruptSelect = pinInt_FallingEdge_c
    }
};

/* Declare Output GPIO pins */
gpioOutputPinConfig_t ledPins[] = {{.gpioPort = gpioPort_A_c, /* RED */
                                    .gpioPin = 31U,
                                    .outputLogic = 0,
                                    .slewRate = pinSlewRate_Slow_c,
                                    .driveStrength = pinDriveStrength_Low_c},
                                   {.gpioPort = gpioPort_A_c, /* GREEN */
                                    .gpioPin = 25U,
                                    .outputLogic = 0,
                                    .slewRate = pinSlewRate_Slow_c,
                                    .driveStrength = pinDriveStrength_Low_c},
                                   {.gpioPort = gpioPort_A_c, /* BLUE */
                                    .gpioPin = 13U,
                                    .outputLogic = 0,
                                    .slewRate = pinSlewRate_Slow_c,
                                    .driveStrength = pinDriveStrength_Low_c}};


