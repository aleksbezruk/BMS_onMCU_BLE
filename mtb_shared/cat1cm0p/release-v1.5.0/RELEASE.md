### CAT1 Cortex M0+ prebuilt images
Prebuilt application images are executed on the Cortex M0+ core of the CAT1 dual-core MCU. The images are provided as C arrays ready to be compiled as part of the Cortex M4/M7 application. The Cortex M0+ application code is placed to internal flash by the Cortex M4/M7 linker script.

### What's Included?

#### CAT1A

* [COMPONENT_CM0P_SLEEP](./COMPONENT_CAT1A/COMPONENT_CM0P_SLEEP/README.md)

    This image starts CM4 core at CY_CORTEX_M4_APPL_ADDR
    and puts CM0+ core into a deep sleep mode.  (Check your device's
    linker file (psoc6) or partition file (Traveo) for the
    location of CY_CORTEX_M4_APPL_ADDR.)

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

#### CAT1C

* [COMPONENT_XMC7xDUAL_CM0P_SLEEP](./COMPONENT_CAT1C/COMPONENT_XMC7xDUAL_CM0P_SLEEP/README.md)

    This image is meant for dual CM7 core (CM7_0/1) XMC7xxx device. This image starts both CM7_0 and CM7_1 core at CY_CORTEX_M7_0_APPL_ADDR and CY_CORTEX_M7_1_APPL_ADDR respectively and puts CM0+ core into a deep sleep mode.

* [COMPONENT_XMC7x_CM0P_SLEEP](./COMPONENT_CAT1C/COMPONENT_XMC7x_CM0P_SLEEP/README.md)

    This image is meant for single CM7 core (CM7_0) XMC7xxx device. This image starts CM7_0 core at CY_CORTEX_M7_0_APPL_ADDR and puts CM0+ core into a deep sleep mode.

### What Changed?

#### v1.5.0
* Adds new support for Traveo II Body Entry 512K and 2M devices, and updates CM0P allocations for TVIIBE1M device.
#### v1.4.0
* Built prebuilts with mtb-pdl-cat1 3.9.0
#### v1.3.0
* Production support added for Traveo II Body Entry devices.
#### v1.2.0
* BLESS FW update to v5.0.12.462
#### v1.1.0
* Driver maintenance and code efficiency enhancement.
* BLESS FW update to v5.0.11.449
#### v1.0.0
* Initial release

### Known Issues in CAT1A M0+ prebuilt images:
   i.  With ble-example-hello-sensor app running in CY8CKIT-062-BLE, when Remote Device A is bonded, Remote Device B would have scanning/connecting issues.
       Recovery is to delete the bonding of Remote Device A.
   ii. While running BLE stress test cases, some times controller stops sending HostMsgFlushRecvCallBack to HOST for the sent messages. This blocks all further operations.
       Recovery is to reset the board.

### Product/Asset Specific Instructions
To pick which image is used in an application update the COMPONENTS variable in the Board Support Package (BSP)'s makefile. Depending on which image is selected, an update to the BSPs linker script may also be required to allocate the proper space for the CM0+ image and that the CM4 has the starting address listed above.

### Supported Software and Tools
This version of the (CAT1 Cortex M0+ prebuilt images) was validated for compatibility with the following Software and Tools:

| Software and Tools                        | Version |
| :---                                      | :----:  |
| GCC Compiler                              | 11.3.1  |
| IAR Compiler                              | 9.3     |
| ARM Compiler 6                            | 6.16    |

### More information
Use the following links for more information, as needed:
* [Cypress Semiconductor, an Infineon Technologies Company](http://www.infineon.com)
* [ModusToolbox](https://www.cypress.com/products/modustoolbox-software-environment)

---
© Cypress Semiconductor Corporation (an Infineon company), 2023.
