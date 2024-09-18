# test-script for QUTest unit testing harness
# see https://www.state-machine.com/qtools/qutest.html

import os
import time

QS_CMD_RED_LED = 0
RED_LED_0N = 1
RED_LED_0FF = 0
RED_LED_UNKNOWN = 2
QSPY_STATUS_ERROR = 0

def get_dictionary():
    dic_file = [f for f in os.listdir('.') if f.endswith('.dic')]
    if len(dic_file) != 1:
        raise ValueError('should be only one dictionary file in the current directory')
    filename = dic_file[0]

    file1 = open(filename, 'r')
    Lines = file1.readlines()
    return Lines

def get_func_id(dic, func):
    for line in dic:
        if func in line:
            fun_id = line[0:10]
    fun_id = int(fun_id, 16)
    return fun_id

def on_reset():
    print("---Reset target completed---")
    print("---Set global filters---")
    glb_filter(GRP_ALL)

# tests...
test(''' QS_onCommand test ''')
dict = get_dictionary()
print(dict)
command(QS_CMD_RED_LED, RED_LED_0N)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(QS_CMD_RED_LED, RED_LED_0FF)
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(QS_CMD_RED_LED, RED_LED_UNKNOWN)
expect("@timestamp Trg-Done QS_RX_COMMAND")

test(''' QS_onReset test ''', NORESET)
func_id = get_func_id(dict, "QS_onReset")
probe('QS_onReset', 1)    # not equal zero to dump memory before reset
command(1, func_id)
expect("@timestamp TstProbe Fun=QS_onReset,Data=1")
time.sleep(500)

test(''' QS_onStartup: inject error ''')
command(4, 1)   # inject error "bsp_status_init_fail"
expect("@timestamp UTEST INJ_ERR_CODE 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
func_id = get_func_id(dict, "__gcov_dump")
print(func_id)
command(1, func_id)
# sleep sometime to save coverage data
time.sleep(500)

# get code coverage data collected by GCOV
test(''' Get code coverage data ''')
func_id = get_func_id(dict, "__gcov_dump")
print(func_id)
command(1, func_id)
# sleep sometime to save coverage data
time.sleep(500)
expect("@timestamp UTEST __gcov_dump")
expect("@timestamp Trg-Done QS_RX_COMMAND")

######################### End of test ####################
