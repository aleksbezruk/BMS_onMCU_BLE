# This is QView customization for a specific application
# (BMS in this case). This extentation animates BMS state on
# QView canvas and allows switches control as well.
#
# Purpose: GUI for BMS functional testing.
#
# This version of the BMS customization uses the application-specific
# trace records QS_USER_00, QS_USER_01 etc. produced when the status of
# a BMS changes.
#
# Functions:
#   1. TODO On/Off switches     -> Basic feature
#   2. Indicate balancers state     -> Basic feature
#   3. TODO Indicate charge/doscharge switches state    -> Basic feature
#   4. TODO Indicate banks voltage & BMS overall charge volatge     -> Basic feature
#   5. TODO Indicate BLE status/state (Connected, Disconnected, Advertising, Write cmd, Read cmd, errors, etc)  -> Basic feature
#   6. TODO Indicate Faults:    -> Advanced feature
#      - Peripherals error status ;
#      - assert exceptions ;
#      - CPU exceptions: HardFault, MemFault, UsageFault, BusFault etc. ;
#      - ADC measurement timeout, invalid "out of range" values (<10 V, >19 V) .
#

from tkinter import *
from tkinter.ttk import * # override the basic Tk widgets with Ttk widgets
from tkinter.simpledialog import *
import time

class BMS:
    def __init__(self):
        # Constants
        self.DISCHARGE_SWITCH_BIT = 0
        self.CHARGE_SWITCH_BIT = 1
        self.BAL1_SWITCH_BIT = 2
        self.BAL2_SWITCH_BIT = 3
        self.BAL3_SWITCH_BIT = 4
        self.BAL4_SWITCH_BIT = 5
        self.DISCHARGE_SWITCH_MASK = (1 << self.DISCHARGE_SWITCH_BIT) & 0xFF
        self.CHARGE_SWITCH_MASK = (1 << self.CHARGE_SWITCH_BIT) & 0xFF
        self.BAL1_SWITCH_MASK = (1 << self.BAL1_SWITCH_BIT) & 0xFF
        self.BAL2_SWITCH_MASK = (1 << self.BAL2_SWITCH_BIT) & 0xFF
        self.BAL3_SWITCH_MASK = (1 << self.BAL3_SWITCH_BIT) & 0xFF
        self.BAL4_SWITCH_MASK = (1 << self.BAL4_SWITCH_BIT) & 0xFF

        # request target reset on startup...
        reset_target()
        #time.sleep(5)
    
        # Variables
        self.SWITCHES_STATE = 0
        #self.dis_all_sw()

        # add commands to the Custom menu...
        QView.custom_menu.add_command(label="Enable Discharge switch",
                                      command=self.en_disch_command)
        QView.custom_menu.add_command(label="Disable Discharge switch",
                                      command=self.dis_disch_command)
        QView.custom_menu.add_command(label="Enable Bank4 balancer switch",
                                      command=self.en_bal4_command)
        QView.custom_menu.add_command(label="Disable Bank4 balancer switch",
                                      command=self.dis_bal4_command)
        QView.custom_menu.add_command(label="Enable Bank3 balancer switch",
                                      command=self.en_bal3_command)
        QView.custom_menu.add_command(label="Disable Bank3 balancer switch",
                                      command=self.dis_bal3_command)
        QView.custom_menu.add_command(label="Enable Bank2 balancer switch",
                                      command=self.en_bal2_command)
        QView.custom_menu.add_command(label="Disable Bank2 balancer switch",
                                      command=self.dis_bal2_command)
        QView.custom_menu.add_command(label="Enable Bank1 balancer switch",
                                      command=self.en_bal1_command)
        QView.custom_menu.add_command(label="Disable Bank1 balancer switch",
                                      command=self.dis_bal1_command)
        QView.custom_menu.add_command(label="Disable All switches",
                                      command=self.dis_all_sw)

        # configure the custom QView.canvas...
        # Note 1: Canvas object -> TKInter object
        # QView._canvas_toplevel = Toplevel()
        # QView.canvas = Canvas(QView._canvas_toplevel)
        QView.show_canvas() # make the canvas visible
        QView.canvas.configure(width=480, height=480)

        # Add items to the canvas
        self.init_bal_state_and_switch_gui(1)   # balancer 1
        self.init_bal_state_and_switch_gui(2)   # balancer 2
        self.init_bal_state_and_switch_gui(3)   # balancer 3
        self.init_bal_state_and_switch_gui(4)   # balancer 4
 
    # on_reset() callback
    def on_reset(self):
        QView.print_text("Note: BMS reset occured")

    # on_run() callback
    # Note 1: 
    # The main qview.py script calls the on_run() callback function in your customization script when 
    # it receives the #QS_QF_RUN trace record from the Target.
    # Note 2:
    # The callback is never reached in BMS app because the BMS app doesn't use QPC framework. 
    # It only uses QS/QSPY module for testing.  
    def on_run(self):
        QView.print_text("BMS running")

    # Balanacer Bank4 set switch callback
    def btn_bal4_callback(self):
        if self.enBal4_sw_state.get() == 1:
            QView.print_text("Enable bank4 balancer")
            self.update_sw_sate(self.BAL4_SWITCH_MASK, 1)
        else:
            QView.print_text("Disable bank4 balancer")
            self.update_sw_sate(self.BAL4_SWITCH_MASK, 0)

    # Balanacer Bank3 set switch callback
    def btn_bal3_callback(self):
        if self.enBal3_sw_state.get() == 1:
            QView.print_text("Enable bank3 balancer")
            self.update_sw_sate(self.BAL3_SWITCH_MASK, 1)
        else:
            QView.print_text("Disable bank3 balancer")
            self.update_sw_sate(self.BAL3_SWITCH_MASK, 0)

    # Balanacer Bank2 set switch callback
    def btn_bal2_callback(self):
        if self.enBal2_sw_state.get() == 1:
            QView.print_text("Enable bank2 balancer")
            self.update_sw_sate(self.BAL2_SWITCH_MASK, 1)
        else:
            QView.print_text("Disable bank2 balancer")
            self.update_sw_sate(self.BAL2_SWITCH_MASK, 0)

    # Balanacer Bank1 set switch callback
    def btn_bal1_callback(self):
        if self.enBal1_sw_state.get() == 1:
            QView.print_text("Enable bank1 balancer")
            self.update_sw_sate(self.BAL1_SWITCH_MASK, 1)
        else:
            QView.print_text("Disable bank1 balancer")
            self.update_sw_sate(self.BAL1_SWITCH_MASK, 0)

    # Update SWITCHES_STATE variable that tracks BMS switches state 
    def update_sw_sate(self, sw_mask, new_sw_state):
        assert new_sw_state == 0 or new_sw_state == 1, "Invalid requested SW_STATE"
        assert sw_mask >= self.DISCHARGE_SWITCH_MASK and sw_mask <= self.BAL4_SWITCH_MASK, "Invalid requested SW_MASK"
        if new_sw_state == 0:
            self.SWITCHES_STATE = self.SWITCHES_STATE & ~sw_mask
        if new_sw_state == 1:
            self.SWITCHES_STATE = self.SWITCHES_STATE | sw_mask
        self.set_sw_state_cmd(self.SWITCHES_STATE)

    # Send cmd to DUT to set new state of switches
    def set_sw_state_cmd(self, sw_states):
        command(5, sw_states) # QS_CMD_BMS_SW_STATES

    # Disable all switches
    def dis_all_sw(self):
        command(5, 0) # QS_CMD_BMS_SW_STATES

    # Enable discharge custom command
    def en_disch_command(self):
        self.update_sw_sate(self.DISCHARGE_SWITCH_MASK, 1)

    # Disable discharge custom command
    def dis_disch_command(self):
        self.update_sw_sate(self.DISCHARGE_SWITCH_MASK, 0)

    # Enable Bank4 balancer switch
    def en_bal4_command(self):
        self.update_sw_sate(self.BAL4_SWITCH_MASK, 1)

    # Disable Bank4 balancer switch
    def dis_bal4_command(self):
        self.update_sw_sate(self.BAL4_SWITCH_MASK, 0)

    # Enable Bank3 balancer switch
    def en_bal3_command(self):
        self.update_sw_sate(self.BAL3_SWITCH_MASK, 1)

    # Disable Bank3 balancer switch
    def dis_bal3_command(self):
        self.update_sw_sate(self.BAL3_SWITCH_MASK, 0)

    # Enable Bank2 balancer switch
    def en_bal2_command(self):
        self.update_sw_sate(self.BAL2_SWITCH_MASK, 1)

    # Disable Bank2 balancer switch
    def dis_bal2_command(self):
        self.update_sw_sate(self.BAL2_SWITCH_MASK, 0)

    # Enable Bank1 balancer switch
    def en_bal1_command(self):
        self.update_sw_sate(self.BAL1_SWITCH_MASK, 1)

    # Disable Bank1 balancer switch
    def dis_bal1_command(self):
        self.update_sw_sate(self.BAL1_SWITCH_MASK, 0)

    # Init GUI object for 'balancer state'
    def init_bal_state_and_switch_gui(self, bal_num):
        # Bank1
        if bal_num == 1:
            self.bank1_bal_state = QView.canvas.create_rectangle(10, 20, 50, 60, outline="blue", fill="white")
            self.enBal1_sw_state = IntVar()
            self.bal1_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank1 balancer",
                                        variable=self.enBal1_sw_state,
                                        command=self.btn_bal1_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal1_btn.place(x=60, y=30)
            self.bal1_btn_label = Label(QView.canvas, textvariable=self.enBal1_sw_state)
            self.bal1_btn_label.place(x=60, y=45)

        # Bank2
        elif bal_num == 2:
            self.bank2_bal_state = QView.canvas.create_rectangle(10, 70, 50, 110, outline="blue", fill="white")
            self.enBal2_sw_state = IntVar()
            self.bal2_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank2 balancer",
                                        variable=self.enBal2_sw_state,
                                        command=self.btn_bal2_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal2_btn.place(x=60, y=80)
            self.bal2_btn_label = Label(QView.canvas, textvariable=self.enBal2_sw_state)
            self.bal2_btn_label.place(x=60, y=95)

        # Bank3
        elif bal_num == 3:
            self.bank3_bal_state = QView.canvas.create_rectangle(10, 120, 50, 160, outline="blue", fill="white")
            self.enBal3_sw_state = IntVar()
            self.bal3_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank3 balancer",
                                        variable=self.enBal3_sw_state,
                                        command=self.btn_bal3_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal3_btn.place(x=60, y=130)
            self.bal3_btn_label = Label(QView.canvas, textvariable=self.enBal3_sw_state)
            self.bal3_btn_label.place(x=60, y=145)

        # Bank4
        elif bal_num == 4:
            self.bank4_bal_state = QView.canvas.create_rectangle(10, 170, 50, 210, outline="blue", fill="white")
            self.enBal4_sw_state = IntVar()
            self.bal4_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank4 balancer",
                                        variable=self.enBal4_sw_state,
                                        command=self.btn_bal4_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal4_btn.place(x=60, y=180)
            self.bal4_btn_label = Label(QView.canvas, textvariable=self.enBal4_sw_state)
            self.bal4_btn_label.place(x=60, y=195)
        else:
            Exception("GUI init error: incorrect bal_num")

    # intercept the QS_USER_00 application-specific packet
    # this packet has the following structure:
    # record-ID, seq-num, Timestamp, zero terminated message string, switches state
    def QS_USER_00(self, packet):
        # unpack: Timestamp->data[0], msg_string->data[1], switches_state->data[2]
        data = qunpack("xxTxZ", packet)
        msg_string = data[1]
        # filter messages received from MAIN task
        if msg_string == "Sys evt, set switches state: ":
            timestamp = data[0]
            switches_state = packet[38]
            # print a message to the text view
            QView.print_text("%010d: switches state = %2d"%(timestamp, switches_state))
    
            # indicate BMS banks status
            if (switches_state & self.BAL1_SWITCH_MASK) == self.BAL1_SWITCH_MASK:
                QView.canvas.itemconfig(self.bank1_bal_state, fill="blue")
            else:
                QView.canvas.itemconfig(self.bank1_bal_state, fill="white")
            
            if (switches_state & self.BAL2_SWITCH_MASK) == self.BAL2_SWITCH_MASK:
                QView.canvas.itemconfig(self.bank2_bal_state, fill="blue")
            else:
                QView.canvas.itemconfig(self.bank2_bal_state, fill="white")

            if (switches_state & self.BAL3_SWITCH_MASK) == self.BAL3_SWITCH_MASK:
                QView.canvas.itemconfig(self.bank3_bal_state, fill="blue")
            else:
                QView.canvas.itemconfig(self.bank3_bal_state, fill="white")

            if (switches_state & self.BAL4_SWITCH_MASK) == self.BAL4_SWITCH_MASK:
                QView.canvas.itemconfig(self.bank4_bal_state, fill="blue")
            else:
                QView.canvas.itemconfig(self.bank4_bal_state, fill="white")
 
#=============================================================================
QView.customize(BMS()) # set the QView customization, see QView._cust 


#=============================================
#  SANDBOX
#=============================================
