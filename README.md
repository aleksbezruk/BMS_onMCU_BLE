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

### III. Tools
1. Software tracing on target -> QSPY & Qview: https://www.state-machine.com/qtools/qpspy.html, <br>
https://www.state-machine.com/qtools/qview.html
2. Python 3 as required by QSpy, Qview, QUtest 
3. Visual Studio Code IDE
3. Arm GNUC for Embedded toolchain

### IV. Testing
1. Unit test harness/framework -> QUtest: https://www.state-machine.com/qtools/qutest.html 


### V. Integration testing framework
TBD. In general, It's preferrable to use framework based on scripting language like Python <br>
I prefer Pyhon based framework.
