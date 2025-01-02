## __*Battery Management System (BMS) on MCU*__

### I. Features
1. Overcharge protection
2. Overdischarge protection
3. Cells balancing
4. 4S scheme - 4*3.7 = 14.8 (16.8)V
5. Controlled via MCU with BLE feature support

### II. Software platform
MCU firmware is untended be simple, there is no need for complex multitasking scheduling algorithm. <br>
Based on this, there is no need to use complex RTOS like FreeRTOS or QP framework. <br>
It can be implemented on baremetal OR using lightweight custom RTOS. <br>
Starting from __baremetal__ implementation.
!!! UPDATE: Cypress/Infineon BLE stack requires multitasking environment and <br> 
the manufacturer BLE stack porting layer is based on RTOS implementation (FreeRTOS, THtreadX etc.). <br>
Decided to use FreeRTOS.  

### III. Tools
1. Software tracing on target -> QSPY & Qview: https://www.state-machine.com/qtools/qpspy.html, <br>
https://www.state-machine.com/qtools/qview.html
2. Python 3 as required by QSpy, Qview, QUtest 
3. Visual Studio Code IDE / ModusToolbox IDE from Cypress
3. Arm GNUC for Embedded toolchain

### IV. Testing
1. Unit test harness/framework -> QUtest: https://www.state-machine.com/qtools/qutest.html 


### V. Integration testing framework
In general, It's preferrable to use framework based on scripting language like Python <br>
pytest framework is used: <br>
> pip install -U pytest <br>
> pip install pytest-dependency <br>
For BLE test the SimpleBLE library is used.

### VI. Code coverage
1. __*gcov*__ code instrumentation feature of GCC compiler is used in order to collect a coverage data. <br>
The coverage data can be collected on both Target & Host systems. <br> 
In the project Target testing is used. To write coverage data to PC from targer, use semihosting feature of debug probe <br>
> When debug session is started, send cmd to debugger via "Debug Console": <br>
> - monitor arm semihosting enable  -> for KitProg debugger
> - set semihosting enable on  -> for JLink
2. To enable GCOV code profiling/coverage: <br>
- define additional compiler flags - <br>
> Example -> CFLAGS= -O0 -Wall -g3 -fprofile-arcs -ftest-coverage  -fprofile-filter-files="main.c qspyHelper.c;BSP.c" ;
-  define additional linker flags - <br>
> Example -> LDFLAGS=-fprofile-arcs -lc -lgcov -lrdimon -specs=rdimon.specs

### VII. ModusToolbox -> system & peripheral config
1. Device Configurator : <br>
> As described in the ModusToolbox™ tools package user guide build system chapter, you can run numerous <br>
> make commands in the application directory, such as launching the Device Configurator. After you have created <br>
> a ModusToolbox™ application, navigate to the application directory and type the following command in the <br>
> appropriate bash terminal window: <br>
> __*make device-configurator*__
2. Bluetooth Configurator <br>
> __*make bt-configurator*__ <br>
3. Project libraries manager <br>
> To add, remove, or modify libraries, open the Library Manager using the following command: <br>
> __*make library-manager*__

### VIII. BLE stack
1. PSoC™ 63 Bluetooth® LE only Legacy Stack => look like is deprecated, Cypress suggest to use a new Stack <br>
> Source: https://github.com/Infineon/bless <br>
> Documentation: https://infineon.github.io/bless/ble_api_reference_manual/html/page_ble_quick_start.html <br>
> New stack: AIROCTM BTSTACK with Bluetooth® LE only (CYW20829, PSoC™ 63, PSoC™ 6 with CYW43xxx Connectivity device) : <br>
>            https://github.com/Infineon/btstack 
2. https://documentation.infineon.com/html/psoc6/jag1667482600571.html
3. https://documentation.infineon.com/html/psoc6/moa1717991724927.html#moa1717991724927 
4. https://github.com/infineon 

### IX. Debugging
1. The main debug probe for now is native Cypress KitProg3. <br>
   There is also an option to use Segger J-Link debug probe.
2. VScode has pluggings to support debugging for Cortex-M. <br>
   Like 'cortex-debug', 'RTOS view' and CPU & MCU peripherals viewers. <br>
   These pluggings makes VSCode suitable as development environment for Cortex-M MCUs.
3. Debug settings defined in the 'launch.json' file.
4. OpenOCD settings defined in such files:
> BMS_PSOC63/openocd.tcl <br>
> BMS_PSOC63/proj_cm4/openocd.tcl <br>
> BMS_PSOC63/proj_cm0/openocd.tcl
5. In order to improve debuggeing session performance (increase speed, reduce latency etc.) <br>
   in 'openocd.tcl' not used RTOS-viewer '-rtos auto ': <br>
> ${TARGET}.cm0 configure -rtos auto -rtos-wipe-on-reset-halt 1 <br>
> ${TARGET}.cm4 configure -rtos auto -rtos-wipe-on-reset-halt 1 <br>
> Note: I believe the performance issue is due to Host-PC performance issue. <br>
>       So upgrading my Host-PC or moving to new machine will fix the issue.

### X. BLE Client
Plan - to develop Desktop and Mobile apps. <br> 
Qt C++ framework & QtBluetooth/SimpleBLE_lib is prefered way to develop cross platform app: <br>
Windows, Linux, MacOS, Android, iOS <br> 
