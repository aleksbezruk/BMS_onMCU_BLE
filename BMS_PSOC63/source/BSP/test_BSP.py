# test-script for QUTest unit testing harness
# see https://www.state-machine.com/qtools/qutest.html

import os
import time

HSIOM_PRT6_PORT_SEL0_REG = 0x40310060
HSIOM_PRT6_PORT_MASK = ((1<<28) | (1<<27) | (1<<26) | (1<<25) | (1<<24))
HSIOM_PRT6_PORT_SHIFT = 24
HSIOM_PRT6_PORT_SEL0_VAL = 0

GPIO_PRT6_OUT_REG = 0x40320300
GPIO_PRT6_OUT_SHIFT = 3
GPIO_PRT6_OUT_MASK = (1<<GPIO_PRT6_OUT_SHIFT)
GPIO_PRT6_OUT_HIGH =  1
GPIO_PRT6_OUT_LOW = 0

GPIO_PRT6_CFG_REG = 0x40320328
GPIO_PRT6_CFG_SHIFT = 12
GPIO_PRT6_CFG_MASK = ((1<<15) | (1<<14) | (1<<13) | (1<<12))
GPIO_PRT6_CFG_VAL = 6

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

def convert_int_to_uit32(val):
    if val >= 0:
        return (val & 0xffffffff)
    else:
        retVal = (val + (1<<32)) & 0xffffffff
        return retVal
    
def read_mcu_reg(addr):
    command(2, addr)
    expect("@timestamp UTEST READ_MCU_REG *")
    print("LAST RECORD IS " + last_rec())
    last = last_rec().split()
    arg1 = int(last[3])
    arg2 = int(last[4])
    mcuReg = convert_int_to_uit32(arg1)
    mcuRegVal = convert_int_to_uit32(arg2)
    expect("@timestamp Trg-Done QS_RX_COMMAND")
    return mcuReg, mcuRegVal

def write_mcu_reg(addr, val):
    val_wr = val & 0xffffffff
    command(3, addr, val_wr)
    expect("@timestamp UTEST WRITE_MCU_REG *")
    expect("@timestamp Trg-Done QS_RX_COMMAND")
    return

# tests...
test(''' BSP_init_led_red test ''')
dict = get_dictionary()
print(dict)
func_id = get_func_id(dict, "BSP_init_led_red")

scenario("Verify MCU registers aceess", NORESET)
note("Write GPIO_PRT6_OUT register", SCREEN)
mcu_reg = read_mcu_reg(GPIO_PRT6_OUT_REG)
val_to_write = ((mcu_reg[1] & ~GPIO_PRT6_OUT_MASK) | GPIO_PRT6_OUT_LOW)
write_mcu_reg(GPIO_PRT6_OUT_REG, val_to_write)
note("Read GPIO_PRT6_OUT register", SCREEN)
mcu_reg = read_mcu_reg(GPIO_PRT6_OUT_REG)
ensure(mcu_reg[0] == GPIO_PRT6_OUT_REG)
ensure(((mcu_reg[1] & GPIO_PRT6_OUT_MASK)>>GPIO_PRT6_OUT_SHIFT) == GPIO_PRT6_OUT_LOW)

scenario("BSP_init_led_red test: call CUT to config MCU registers", NORESET)
command(1, func_id)
expect("@timestamp UTEST BSP_init_led_red")
expect("@timestamp Trg-Done QS_RX_COMMAND")

scenario("BSP_init_led_red test: verify registers", NORESET)
# read MCU regs to verify -> GPIO_PRT6, HSIOM_PRT6_PORT_SEL0 :
# 1. HSIOM_PRT6_PORT_SEL0: addr=0x40310060, bits [28:24] = 0x00 (GPIO controls "out") ;
# 2. Out value=1 -> GPIO_PRT6_OUT: addr=0x40320300, bits[3]=1
# 3. driveMode=CY_GPIO_DM_STRONG_IN_OFF(0x06) -> GPIO_PRT6_CFG: addr= 0x40320328, bits[15]=0, bits[14:12]=6
note("Verify HSIOM_PRT6_PORT_SEL0 register", SCREEN)
mcu_reg = read_mcu_reg(HSIOM_PRT6_PORT_SEL0_REG)
ensure(mcu_reg[0] == HSIOM_PRT6_PORT_SEL0_REG)
ensure(((mcu_reg[1] & HSIOM_PRT6_PORT_MASK)>>HSIOM_PRT6_PORT_SHIFT) == HSIOM_PRT6_PORT_SEL0_VAL)
note("Verify GPIO_PRT6_OUT register", SCREEN)
mcu_reg = read_mcu_reg(GPIO_PRT6_OUT_REG)
ensure(mcu_reg[0] == GPIO_PRT6_OUT_REG)
ensure(((mcu_reg[1] & GPIO_PRT6_OUT_MASK)>>GPIO_PRT6_OUT_SHIFT) == GPIO_PRT6_OUT_HIGH)
note("Verify GPIO_PRT6_CFG register", SCREEN)
mcu_reg = read_mcu_reg(GPIO_PRT6_CFG_REG)
ensure(mcu_reg[0] == GPIO_PRT6_CFG_REG)
ensure(((mcu_reg[1] & GPIO_PRT6_CFG_MASK)>>GPIO_PRT6_CFG_SHIFT) == GPIO_PRT6_CFG_VAL)

test(''' BSP_init_led_green test: call CUT to config MCU registers ''', NORESET)
func_id = get_func_id(dict, "BSP_init_led_green")
command(1, func_id)
expect("@timestamp UTEST BSP_init_led_green")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("BSP_power_init: call CUT to config MCU registers", NORESET)
# Just code execution test, actual veriification should be done 
# by functional and characterization tests (power consumption, power save mode transition etc.)
func_id = get_func_id(dict, "BSP_power_init_test")
note("BSP_power_init: normal path execution", SCREEN)
command(1, func_id)
expect("@timestamp BSP BSP_power_init_test RETURN_VAL 0")
expect("@timestamp UTEST BSP_power_init_test")
expect("@timestamp Trg-Done QS_RX_COMMAND")
##########################################################
note("BSP_power_init: FAILED path execution", SCREEN)
#func_to_probe = get_func_id(dict, "BSP_power_init")
probe('BSP_power_init', 255)    # status isn't success
command(1, func_id)
expect("@timestamp TstProbe Fun=BSP_power_init,Data=255")
expect("@timestamp BSP BSP_power_init_test RETURN_VAL 255")
expect("@timestamp UTEST BSP_power_init_test")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test(''' BSP_clock_setWaitStates ''')

