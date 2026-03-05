## __*Battery Management System (BMS) on MCU*__

### 1. Features </span> 
1.1 🔋 Overcharge protection <br>
1.2 🔌 Overdischarge protection <br>
1.3 ⚖️ Cell balancing algorithm <br>
1.4 🔢 4S battery configuration: 4 × 3.7 V (4.2 V max) = 14.8 V nominal (16.8 V max) <br>
1.5 📡 MCU-controlled with BLE support <br>
1.6 🖥️📱 BLE desktop & mobile application <br>
    Available in a separate repository: https://github.com/aleksbezruk/BMS_app.git <br>
1.7 Schematic/block diagram <br>
![BMS high level schematic](docs/bms_schematic.png)

_________________________________________________________________________________________________

### 2. Software platform
MCU firmware uses multitasking scheduling algorithm, since BLE stack requires multitasking environment and <br> 
the manufacturer's BLE stack porting layer is based on RTOS implementation (FreeRTOS, ThtreadX etc.). <br>
Decided to use FreeRTOS. <br>
Development environment - Host PC/laptop with Ubuntu 22.04 or 24.04 <br>
The software platform supports several ports: <br> 
- PSOC63: tested with CYBLE-416045-02 -> CY8CPROTO-063-BLE DK <br>
![PSOC63 DK](docs/PSOC63-DK.png)
- QN9080C: tested with QN9080-DK v1.3 <br>
![QN9080 DK](docs/QN9080-DK_v1_3.png) <br>

<span style="color: #4FC3F7;">Firmware architecture diagram is provided below.</span> 
![BMS_MCU_firmware-arch.png](docs/BMS_MCU_firmware-arch.png) <br>

_________________________________________________________________________________________________

### 3. Tools
3.1 Software tracing on target -> QSPY & Qview: <br>
> https://www.state-machine.com/qtools/qpspy.html <br>

3.2 Python 3 is required by QSpy, Qview, QUtest <br>

3.3 Visual Studio Code IDE / ModusToolbox IDE from Cypress <br>

3.4 Arm GNUC for Embedded toolchain <br>

3.5 Additional build scripts are located in __**CI-CD/build**__ <br>

_________________________________________________________________________________________________

### 4. Testing
Unit test harness/framework -> QUtest: https://www.state-machine.com/qtools/qutest.html <br>

_________________________________________________________________________________________________


### 5. Integration testing framework
In general, it's preferrable to use framework based on scripting language like Python. <br>
For now __**pytest**__ framework is used: <br>
> pip install -U pytest <br>
> pip install pytest-dependency <br>
For BLE testomg the SimpleBLE library is used: <br>
> check installed version: pip show simplepyble <br>
> install: pip install simplepyble==0.10.3 OR pip install simplepyble==0.10.3 --break-system-packages <br>

_________________________________________________________________________________________________

### 6. Code coverage
__*gcov*__ code instrumentation feature of GCC compiler is used in order to collect a coverage data. <br>
The coverage data can be collected on both Target & Host systems. <br> 
For now target testing is used. To get coverage data from target board to Host PC, use semihosting feature of debug probe. <br>
> When debug session is started, send cmd to debugger via "Debug Console": <br>
> - monitor arm semihosting enable  -> for KitProg debugger <br>
> - set semihosting enable on  -> for JLink <br>

To enable GCOV code profiling/coverage: <br>
- define additional compiler flags: <br>
> Example -> CFLAGS= -O0 -Wall -g3 -fprofile-arcs -ftest-coverage  -fprofile-filter-files="main.c qspyHelper.c;BSP.c" ;
-  define additional linker flags: <br>
> Example -> LDFLAGS=-fprofile-arcs -lc -lgcov -lrdimon -specs=rdimon.specs

### 7. ModusToolbox -> system & peripheral config
7.1 Install ModusToolbox Tools Package 3.2 from Infineon site.
> https://softwaretools.infineon.com/tools/com.ifx.tb.tool.modustoolbox <br>
> before project build, get project libraries: make getlibs <br>

7.2 Device Configurator <br>
> As described in the ModusToolbox™ tools package user guide build system chapter, you can run numerous <br>
> make commands in the application directory, such as launching the Device Configurator. Navigate to the application <br> directory and type the following command in the appropriate bash terminal window: <br>
> __*make device-configurator*__ <br>

7.3 Bluetooth Configurator <br>
> __*make bt-configurator*__ <br>

7.4 Project libraries manager <br>
> To add, remove, or modify libraries, open the Library Manager using the following command: <br>
> __*make library-manager*__ <br>

_________________________________________________________________________________________________

### 8. BLE profile
8.1 Cypress BLE stack <br> 
PSoC™ 63 Bluetooth® LE only Legacy Stack => look like is deprecated, Cypress suggest to use a new Stack <br>
> Source: https://github.com/Infineon/bless <br>
> Documentation: https://infineon.github.io/bless/ble_api_reference_manual/html/page_ble_quick_start.html <br>
> New stack: AIROCTM BTSTACK with Bluetooth® LE only (CYW20829, PSoC™ 63, PSoC™ 6 with CYW43xxx Connectivity device) : <br>
> https://github.com/Infineon/btstack <br>

8.2 Cypress BLE stack doc: https://documentation.infineon.com/html/psoc6/jag1667482600571.html <br>

8.3 Cypress BLE stack doc: https://documentation.infineon.com/html/psoc6/moa1717991724927.html#moa1717991724927 <br>

8.4 Cypress BLE stack doc: https://github.com/infineon <br>

8.5 The BLE profile consist of such main services: __**Battery Service (BAS)**__ and __**Analog Input/Output Signals (AIOS)**__ . <br>
- <span style="color: #4FC3F7;">BAS:</span> <br>
- BAS UUID: 0x180F; <br>
- battery level characteristic UUID: 0x2A19; <br>
- battery level characteristic properties: read and notify. <br>
- <span style="color: #4FC3F7;">AIOS:</span> <br>
- AIOS UUID: 0x1815; <br>
- switch IO state characteristic value UUID: 37AF9AE2-211D-4436-9D26-3A9ED02EFEEA <br>
- switch IO state characteristic properties: read, write and notify. <br>
- full VBAT characteristic value UUID: 170AD8DB-5244-4926-963E-417099122BBA <br>
- full VBAT characteristic properties: read. <br>
- bank1 characteristic value UUID: 170AD8DB-5244-4926-963E-417099122BB1 <br>
- bank1 characteristic properties: read. <br>
- bank2 characteristic value UUID: 170AD8DB-5244-4926-963E-417099122BB2 <br>
- bank2 characteristic properties: read. <br>
- bank3 characteristic value UUID: 170AD8DB-5244-4926-963E-417099122BB3 <br>
- bank3 characteristic properties: read. <br>
- bank4 characteristic value UUID: 170AD8DB-5244-4926-963E-417099122BB4 <br>
- bank4 characteristic properties: read. <br>

_________________________________________________________________________________________________

### 9. Debugging
9.1 The main debug probe for now is native Cypress KitProg3. <br>
   There is also an option to use Segger J-Link debug probe. <br>

9.2 VScode has pluggings to support debugging for Cortex-M. <br>
   Like 'cortex-debug', 'RTOS view' and CPU & MCU peripherals viewers. <br>
   These pluggings makes VSCode suitable as development environment for firmware based on Cortex-M MCUs. <br>

9.3 Debug settings defined in the 'launch.json' file. <br>

_________________________________________________________________________________________________

### 10. BLE Client
Qt C++ framework & QtBluetooth/SimpleBLE_lib is prefered way to develop cross platform app: <br>
Windows, Linux, MacOS, Android, iOS <br> 
See https://github.com/aleksbezruk/BMS_app.git .
_________________________________________________________________________________________________

## 11. QN908x port details
One of the main goal of the project is to develop BMS firmware that is ported to several MCUs from different vendors: <br>
- PSOC63 Cypress/Infineon ; <br>
- QN908x NXP ; <br>
- NRF52840 Nordic ; <br>
- an maybe more . <br>

To achieve this goal a HAL layer is provided. <br>
QN908x port development -> QN9080 DK v1.3. <br>

To accelerate firmware porting to QN908x the MCU Config tools will be used (power, clock, pin, peripherals config). <br>
*.mex - MCU Exported Configuration file - is used by NXP tool. <br>
> See https://www.nxp.com/design/design-center/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-config-tools-pins-clocks-and-peripherals:MCUXpresso-Config-Tools <br>
_________________________________________________________________________________________________

## 12. Nordic NRF52840 port details
For now NRF52840 port is suspended, this option is reserved for future.
