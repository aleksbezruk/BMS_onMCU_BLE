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
#   1. On/Off switches     -> Basic feature
#   2. Indicate balancers state     -> Basic feature
#   3. Indicate charge/doscharge switches state    -> Basic feature
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
import ctypes

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
    
        # Variables
        self.SWITCHES_STATE = 0

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
        QView.canvas.configure(width=640, height=640)

        # Add items to the canvas
        self.init_bal_state_and_switch_gui(1)   # balancer 1
        self.init_bal_state_and_switch_gui(2)   # balancer 2
        self.init_bal_state_and_switch_gui(3)   # balancer 3
        self.init_bal_state_and_switch_gui(4)   # balancer 4
        self.init_disch_state_and_switch_gui()
        self.init_charge_state_and_switch_gui()
        self.init_full_vbat_gui()
 
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

    # Discharge set SW callback
    def btn_disch_callback(self):
        if self.disch_sw_state.get() == 1:
            QView.print_text("Enable Disch SW")
            self.update_sw_sate(self.DISCHARGE_SWITCH_MASK, 1)
        else:
            QView.print_text("Disable Disch SW")
            self.update_sw_sate(self.DISCHARGE_SWITCH_MASK, 0)

    # Charge set SW callback
    def btn_charge_callback(self):
        if self.charge_sw_state.get() == 1:
            QView.print_text("Enable charge SW")
            self.update_sw_sate(self.CHARGE_SWITCH_MASK, 1)
        else:
            QView.print_text("Disable charge SW")
            self.update_sw_sate(self.CHARGE_SWITCH_MASK, 0)

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
            self.bank1_bal_state = QView.canvas.create_rectangle(70, 20, 110, 60, outline="blue", fill="white")
            QView.canvas.create_text(30, 25, text="Bank 1", fill="#004D40", font=('freemono bold',13))
            self.b1_volt_out_obj = QView.canvas.create_text(30, 45, text="? mV", fill="magenta", font=('freemono bold',12))
            self.enBal1_sw_state = IntVar()
            self.bal1_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank1 balancer",
                                        variable=self.enBal1_sw_state,
                                        command=self.btn_bal1_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal1_btn.place(x=120, y=30)
            self.bal1_btn_label = Label(QView.canvas, textvariable=self.enBal1_sw_state)
            self.bal1_btn_label.place(x=120, y=45)

        # Bank2
        elif bal_num == 2:
            self.bank2_bal_state = QView.canvas.create_rectangle(70, 90, 110, 130, outline="blue", fill="white")
            QView.canvas.create_text(30, 95, text="Bank 2", fill="#004D40", font=('freemono bold',13))
            self.b2_volt_out_obj = QView.canvas.create_text(30, 115, text="? mV", fill="magenta", font=('freemono bold',12))
            self.enBal2_sw_state = IntVar()
            self.bal2_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank2 balancer",
                                        variable=self.enBal2_sw_state,
                                        command=self.btn_bal2_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal2_btn.place(x=120, y=100)
            self.bal2_btn_label = Label(QView.canvas, textvariable=self.enBal2_sw_state)
            self.bal2_btn_label.place(x=120, y=115)

        # Bank3
        elif bal_num == 3:
            self.bank3_bal_state = QView.canvas.create_rectangle(70, 160, 110, 200, outline="blue", fill="white")
            QView.canvas.create_text(30, 165, text="Bank 3", fill="#004D40", font=('freemono bold',13))
            self.b3_volt_out_obj = QView.canvas.create_text(30, 185, text="? mV", fill="magenta", font=('freemono bold',12))
            self.enBal3_sw_state = IntVar()
            self.bal3_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank3 balancer",
                                        variable=self.enBal3_sw_state,
                                        command=self.btn_bal3_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal3_btn.place(x=120, y=170)
            self.bal3_btn_label = Label(QView.canvas, textvariable=self.enBal3_sw_state)
            self.bal3_btn_label.place(x=120, y=185)

        # Bank4
        elif bal_num == 4:
            self.bank4_bal_state = QView.canvas.create_rectangle(70, 230, 110, 270, outline="blue", fill="white")
            QView.canvas.create_text(30, 235, text="Bank 4", fill="#004D40", font=('freemono bold',13))
            self.b4_volt_out_obj = QView.canvas.create_text(30, 255, text="? mV", fill="magenta", font=('freemono bold',12))
            self.enBal4_sw_state = IntVar()
            self.bal4_btn = Checkbutton(QView.canvas,
                                        text="Enable Bank4 balancer",
                                        variable=self.enBal4_sw_state,
                                        command=self.btn_bal4_callback,
                                        onvalue = 1,
                                        offvalue = 0)
            self.bal4_btn.place(x=120, y=240)
            self.bal4_btn_label = Label(QView.canvas, textvariable=self.enBal4_sw_state)
            self.bal4_btn_label.place(x=120, y=255)
        else:
            Exception("GUI init error: incorrect bal_num")

    # Init GUI for Discharge switch
    def init_disch_state_and_switch_gui(self):
        self.disch_state = QView.canvas.create_rectangle(70, 300, 110, 340, outline="blue", fill="white")
        QView.canvas.create_text(30, 305, text="Disch", fill="#004D40", font=('freemono bold',13))
        self.disch_sw_state = IntVar()
        self.disch_btn = Checkbutton(QView.canvas,
                                    text="Disch SW",
                                    variable=self.disch_sw_state,
                                    command=self.btn_disch_callback,
                                    onvalue = 1,
                                    offvalue = 0)
        self.disch_btn.place(x=120, y=300)
        self.disch_btn_label = Label(QView.canvas, textvariable=self.disch_sw_state)
        self.disch_btn_label.place(x=120, y=310)

    # Init GUI for Charge switch
    def init_charge_state_and_switch_gui(self):
        self.charge_state = QView.canvas.create_rectangle(70, 370, 110, 410, outline="blue", fill="white")
        QView.canvas.create_text(30, 375, text="Charge", fill="#004D40", font=('freemono bold',13))
        self.charge_sw_state = IntVar()
        self.charge_btn = Checkbutton(QView.canvas,
                                    text="Charge SW",
                                    variable=self.charge_sw_state,
                                    command=self.btn_charge_callback,
                                    onvalue = 1,
                                    offvalue = 0)
        self.charge_btn.place(x=120, y=370)
        self.charge_btn_label = Label(QView.canvas, textvariable=self.charge_sw_state)
        self.charge_btn_label.place(x=120, y=380)

    # Init GUI for full VBAT indication
    def init_full_vbat_gui(self):
        self.full_vbat_out_obj = QView.canvas.create_text(100, 440, text="Full VBAT = ? mV", fill="magenta", font=('freemono bold',14))

    # intercept the QS_USER_00 application-specific packet(s) from Main task
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
            timestamp_str = "%010d:"%(timestamp)
            string =  "switches state = %2d"%(switches_state)
            self.qview_custom_print(timestamp_str, string)

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
    
            if (switches_state & self.DISCHARGE_SWITCH_MASK) == self.DISCHARGE_SWITCH_MASK:
                QView.canvas.itemconfig(self.disch_state, fill="blue")
            else:
                QView.canvas.itemconfig(self.disch_state, fill="white")

            if (switches_state & self.CHARGE_SWITCH_MASK) == self.CHARGE_SWITCH_MASK:
                QView.canvas.itemconfig(self.charge_state, fill="blue")
            else:
                QView.canvas.itemconfig(self.charge_state, fill="white")

        elif msg_string == "Banks volt: ":
            # packet structure: record-ID, seq-num, Timestamp, msg string, banks volt with format bytes
            # unpack: Timestamp->tmp_data[0], msg_string->tmp_data[1], bank1->tmp_data[3], ... bank4->tmp_data[9]
            #         fullVbat->tmp_data[11]
            tmp_data = qunpack("xxTZBHBHBHBHBH", packet)
            timestamp = tmp_data[0]
            #fmt = tmp_data[2]
            #b1 = tmp_data[3]
            #print("fmt:")
            #print(fmt)
            #print("b1:")
            #print(b1)
            b1 = ctypes.c_short(tmp_data[3]).value  # int16_t
            b2 = ctypes.c_short(tmp_data[5]).value
            b3 = ctypes.c_short(tmp_data[7]).value
            b4 = ctypes.c_short(tmp_data[9]).value
            fullVbat = ctypes.c_short(tmp_data[11]).value
            # print a message to the text view
            timestamp_str = "%010d:"%(timestamp)
            string =  "banks voltage in mV: b1=%5d, b2=%5d, b3=%5d, b4=%5d fullBat=%5d"%(b1, b2, b3, b4, fullVbat)
            self.qview_custom_print(timestamp_str, string)
            # update banks volt on Canvas
            QView.canvas.itemconfig(self.b1_volt_out_obj, text="%5d"%(b1) + " mV")
            QView.canvas.itemconfig(self.b2_volt_out_obj, text="%5d"%(b2) + " mV")
            QView.canvas.itemconfig(self.b3_volt_out_obj, text="%5d"%(b3) + " mV")
            QView.canvas.itemconfig(self.b4_volt_out_obj, text="%5d"%(b4) + " mV")
            QView.canvas.itemconfig(self.full_vbat_out_obj, text="Full VBAT = %5d mV"%(fullVbat))
 
    def qview_custom_print(self, timestamp_str, string):
        QView._text.delete(1.0, QView._text_lines)
        QView._text.insert(END, "\n")
        QView._text.tag_add("highlight_time", "1.0", "1.10")
        QView._text.insert(END, timestamp_str, "highlight_time")
        QView._text.tag_add("highlight_string", "1.11", "1.13")
        QView._text.insert(END, string, "highlight_string")
        QView._text.tag_configure("highlight_time", foreground = "green", background ="#ccc")
        QView._text.tag_configure("highlight_string", foreground = "blue")
        if QView._scroll_text.get():
            QView._text.yview_moveto(1) # scroll to the bottom

#=============================================================================
QView.customize(BMS()) # set the QView customization, see QView._cust 


#=============================================
#  SANDBOX
#=============================================
