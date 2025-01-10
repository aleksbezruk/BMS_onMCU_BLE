# PSoC 6 Cortex M0+ BLESS Controller pre-built image (CM0P_BLESS_OTA)

### Overview
This image is same as CM0P_BLESS except that CM4 app start address would be (CY_FLASH_BASE + 0x20000U + 0x20000U),
Since MCUBootApp resides at 0x10000000 (start of flash) and is 0x00018000 bytes long.
Pre-compiled BLESS Controller image executed on the Cortex M0+ core of the PSoC 6 dual-core MCU.
The image is provided as C array ready to be compiled as part of the Cortex M4 application.
The Cortex M0+ application code is placed to internal flash by the Cortex M4 linker script.

This image is used only in BLE dual CPU mode. In this mode, the BLE functionality is split between
CM0+ (controller) and CM4 (host). It uses IPC for communication between two CPU cores where both the
controller and host run:

    -------------------------------          ------------------------------------
    |    CM0p (pre-built image)   |          |              CM4                 |
    | --------------------  ----- |          | -----  ------------------------  |
    | |                  |  | H | |   IPC    | |   |  |  BLE Application     |  |
    | | BLE Controller   |--| c | |<-------->| |   |  ------------------------  |
    | |       (LL)       |  | I | |(commands,| |   |          |            |    |
    | |                  |  ----- |  events) | |   |  -----------------    |    |
    | --------------------        |          | | H |  |  BLE Profiles |    |    |
    -------------------------------          | | C |  -----------------    |    |
                  |                          | | I |          |            |    |
       ------------------------              | |   |  ------------------------- |
       |       BLE HW         |              | |   |--|  BLE Host (GAP, L2CAP,| |
       ------------------------              | |   |  |  SM, ATT)             | |
                                             | |   |  ------------------------- |
                                             | -----                            |
                                             ------------------------------------



BLESS Controller pre-built image executes the following steps:
- configures BLESS interrupt
- registers IPC-pipe callback for BLE middleware; the BLE middleware uses this callback to
  initialize and enable the BLE controller when BLE middleware operates in BLE dual CPU mode
- starts CM4 core at CY_CORTEX_M4_APPL_ADDR=0x10020000
- goes to the while loop where processes BLE controller events and puts the CM0+ core into Deep Sleep.


Flash Memory Layout
  MCUBootApp resides at 0x10000000 (start of flash) and is 0x00018000 bytes long.
  This needs to match the flash base address from the CM4 application's linker script.
  0x10000000 ----------------------------------------
             |  MCUBootApp                          |
  0x10020000 ---------------------------------------- ------------------------------------\
             | Required MCUBootApp Header           | (added in signing step of User App)  \
  0x10020400 |      ------------------              |                                       \
             |  Application cm0p bless code         |                                         --> Primary slot
  0x10040000 |      ------------------              |                                       /
             |  Application cm4 code (User App)     |                                      /
             |                                      |                                     /
  0x10080000 ---------------------------------------- ------------------------------------
             |                                      |
             |                                      | Secondary Slot
             |                                      |
  0x100e0000 ---------------------------------------- --------------
             |  Scratch Area  (For MCUBootApp)      |
  0x100e4000 ----------------------------------------
             |  Status Area  (For MCUBootApp)       |
  0x100e8000 ----------------------------------------
### New in this image
- This image is same as CM0P_BLESS except that CM4 app start address is defined as (CY_FLASH_BASE + 0x20000U + 0x20000U), as explained above.


The revision history of the PSoC 6 BLE Middleware is also available on the [API Reference Guide Changelog](https://cypresssemiconductorco.github.io/bless/ble_api_reference_manual/html/page_group_ble_changelog.html).


### Usage
By using this image, CM4 app start address would be (CY_FLASH_BASE + 0x20000U + 0x20000U).
This image to be used along with OTA application's linker script

To use this image in the custom BSP, adjust the BSP target makefile to
add the COMPONENT_CM0P_BLESS_OTA directory to the list of components
discovered by ModusToolbox build system:

```
COMPONENTS+=CM0P_BLESS_OTA
```



Make sure there is a single CM0P_* component included in the COMPONENTS list
(it might be needed to remove CM0P_SLEEP from the list of standard BSP components).

---
Copyright (c) Cypress Semiconductor Corporation, 2022.
