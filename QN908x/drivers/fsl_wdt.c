/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_wdt.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
* Prototypes
******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
void WDT_Init(WDT_Type *base, const wdt_config_t *config)
{
    assert(config);

    CLOCK_EnableClock(kCLOCK_Wdt);

    WDT_Unlock(base);

    base->LOAD = config->loadValue;

    if (config->enableWdtReset)
    {
        base->CTRL = WDT_CTRL_RESEN_MASK | WDT_CTRL_INTEN_MASK;
    }
    else
    {
        base->CTRL = WDT_CTRL_INTEN_MASK;
    }

    WDT_Lock(base);
}

void WDT_Deinit(WDT_Type *base)
{
    WDT_Unlock(base);
    base->CTRL = 0;
    WDT_Lock(base);

    CLOCK_DisableClock(kCLOCK_Wdt);
}

void WDT_GetDefaultConfig(wdt_config_t *config)
{
    assert(config);

    config->enableWdtReset = true;
    config->loadValue = 0xffffffffU;
}
