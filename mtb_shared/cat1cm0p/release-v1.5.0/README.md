# CAT1 Cortex M0+ prebuilt images

### Overview

Prebuilt application images are executed on the Cortex M0+ core of the CAT1 dual-core MCU.
The images are provided as C arrays ready to be compiled as part of the Cortex M4/M7 application.
The Cortex M0+ application code is placed to internal flash by the Cortex M4/M7 linker script.

Note: Each application image has a variant based on the hardware die (e.g.
psoc6_01, psoc6_02, psoc6_03, xmc7100, xmc7200, tviibe1m, tviibe4m) it is supported on. 
An #ifdef at the top of each .c file automatically controls which version is used so there 
is no need to specify a particular image.

### CAT1A Images

* [COMPONENT_CM0P_SLEEP](./COMPONENT_CAT1A/COMPONENT_CM0P_SLEEP/README.md)

    This image starts CM4 core at CY_CORTEX_M4_APPL_ADDR=0x10002000
    and puts CM0+ core into a deep sleep mode.

* [COMPONENT_CM0P_CRYPTO](./COMPONENT_CAT1A/COMPONENT_CM0P_CRYPTO/README.md)

    This image starts crypto server on CM0+ core,
    starts CM4 core at CY_CORTEX_M4_APPL_ADDR=0x1000A000
    and puts CM0+ core into a deep sleep mode.

* [COMPONENT_CM0P_BLESS](./COMPONENT_CAT1A/COMPONENT_CM0P_BLESS/README.md)

    This image starts BLE controller on CM0+ core,
    starts CM4 core at CY_CORTEX_M4_APPL_ADDR=0x10020000
    and puts CM0+ core into a deep sleep mode.

* [COMPONENT_CM0P_SECURE](./COMPONENT_CAT1A/COMPONENT_CM0P_SECURE/README.md)

    This image starts CM4 core at address corresponding
    to Secure Boot policy, sets required security settings,
    initializes and executes code of Protected Register Access
    driver, puts CM0+ core into a deep sleep mode.

### CAT1C Images

* [COMPONENT_XMC7xDUAL_CM0P_SLEEP](./COMPONENT_CAT1C/COMPONENT_XMC7xDUAL_CM0P_SLEEP/README.md)

    This image is meant for dual CM7 core (CM7_0/1) XMC7xxx device. This image starts both CM7_0 and CM7_1 core at CY_CORTEX_M7_0_APPL_ADDR and CY_CORTEX_M7_1_APPL_ADDR respectively and puts CM0+ core into a deep sleep mode.

* [COMPONENT_XMC7x_CM0P_SLEEP](./COMPONENT_CAT1C/COMPONENT_XMC7x_CM0P_SLEEP/README.md)

    This image is meant for single CM7 core (CM7_0) XMC7xxx device. This image starts CM7_0 core at CY_CORTEX_M7_0_APPL_ADDR and puts CM0+ core into a deep sleep mode.

### More information
Use the following links for more information, as needed:
* [Cypress Semiconductor, an Infineon Technologies Company](http://www.infineon.com)
* [ModusToolbox](https://www.cypress.com/products/modustoolbox-software-environment)

---
© Cypress Semiconductor Corporation (an Infineon company), 2023.
