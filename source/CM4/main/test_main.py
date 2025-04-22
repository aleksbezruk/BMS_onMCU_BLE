# test-script for QUTest unit testing harness
# see https://www.state-machine.com/qtools/qutest.html

import os
import time

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
test(''' mainTask_ test ''')
dict = get_dictionary()
print(dict)
func_id = get_func_id(dict, "mainTask_")
print(func_id)
command(1, func_id)
expect("@timestamp UTEST mainTask_")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# get code coverage data collected by GCOV
test(''' Get code coverage data ''', NORESET)
func_id = get_func_id(dict, "__gcov_dump")
print(func_id)
command(1, func_id)
# sleep sometime to save coverage data
time.sleep(500)
expect("@timestamp UTEST __gcov_dump")
expect("@timestamp Trg-Done QS_RX_COMMAND")
