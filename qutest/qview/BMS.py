# This is QView customization for a specific application
# (BMS in this case). This example animates BMS state on
# QView canvas.
#
# This version of the BMS customization uses the application-specific
# trace records QS_USER_x (???) produced when the status of
# a BMS changes.

class BMS:
    def __init__(self):
        # request target reset on startup...
        reset_target()
    
        # add commands to the Custom menu...
        QView.custom_menu.add_command(label="Enable Discharge switch",
                                      command=self.en_disch_command)
        QView.custom_menu.add_command(label="Disable Discharge switch",
                                      command=self.dis_disch_command)
        QView.custom_menu.add_command(label="Enable Bank4 balancer switch",
                                      command=self.en_bal4_command)
        QView.custom_menu.add_command(label="Disable Bank4 balancer switch",
                                      command=self.dis_bal4_command)
    
        # configure the custom QView.canvas...
        QView.show_canvas() # make the canvas visible
        QView.canvas.configure(width=400, height=260)
    
        # tuple of activity images (correspond to self._philo_state)
        #self._act_img = (
        #    PhotoImage(file=HOME_DIR + "/inline/thinking.gif"),
        #    PhotoImage(file=HOME_DIR + "/inline/hungry.gif"),
        #    PhotoImage(file=HOME_DIR + "/inline/eating.gif"),
        #)
        # tuple of philo canvas images (correspond to self._philo_obj)
        #self._philo_img = (\
        #    QView.canvas.create_image(190,  57, image=self._act_img[0]),
        #    QView.canvas.create_image(273, 100, image=self._act_img[0]),
        #    QView.canvas.create_image(237, 185, image=self._act_img[0]),
        #    QView.canvas.create_image(146, 185, image=self._act_img[0]),
        #    QView.canvas.create_image(107, 100, image=self._act_img[0])
        #)

        # button images for UP and DOWN
        #self.img_UP  = PhotoImage(file=HOME_DIR + "/inline/BTN_UP.gif")
        #self.img_DWN = PhotoImage(file=HOME_DIR + "/inline/BTN_DWN.gif")
    
        # images of a button for pause/serve
        #self.btn = QView.canvas.create_image(200, 120, image=self.img_UP)
        #QView.canvas.tag_bind(self.btn, "<ButtonPress-1>", self.cust_pause)
 
 
    # on_reset() callback
    def on_reset(self):
        # clear the lists
        #self._philo_obj   = [0, 0, 0, 0, 0]
        #self._philo_state = [0, 0, 0]
        print("BMS reset occured")

    # on_run() callback
    def on_run(self):
        print("BMS running")
        #glb_filter("QS_USER_00")
        
        # NOTE: the names of objects for loc_filter() and current_obj()
        # must match the QS Object Dictionaries produced by the application.
        #current_obj(OBJ_AO, "Table_inst")
        #loc_filter(IDS_AO)

        # turn lists into tuples for better performance
        #self._philo_obj = tuple(self._philo_obj)
        #self._philo_state = tuple(self._philo_state)

    # Enable discharge custom command
    def en_disch_command(self):
        command(5, 0x01)   # QS_CMD_BMS_SW_STATES

    # Disable discharge custom command
    def dis_disch_command(self):
        command(5, 0x00)   # QS_CMD_BMS_SW_STATES

    # Enable Bank4 balancer switch
    def en_bal4_command(self):
        command(5, 0x20) # QS_CMD_BMS_SW_STATES

    # Disable Bank4 balancer switch
    def dis_bal4_command(self):
        command(5, 0x00) # QS_CMD_BMS_SW_STATES

    # example of a custom interaction with a canvas object (pause/serve)
    def cust_pause(self, event):
        if QView.canvas.itemcget(self.btn, "image") != str(self.img_UP):
            QView.canvas.itemconfig(self.btn, image=self.img_UP)
            post("SERVE_SIG")
            QView.print_text("Table SERVING")
        else:
            QView.canvas.itemconfig(self.btn, image=self.img_DWN)
            post("PAUSE_SIG")
            QView.print_text("Table PAUSED")

            # intercept the QS_USER_00 application-specific packet
            # this packet has the following structure (see bsp.c:displayPhilStat()):
            # record-ID, seq-num, Timestamp, format-byte, Philo-num,
            #    format-bye, Zero-terminated string (status)
    # def QS_USER_00(self, packet):
    #     # unpack: Timestamp->data[0], Philo-num->data[1], status->data[3]
    #     data = qunpack("xxTxBxZ", packet)
    #     i = data[1]
    #     j = ("t", "h", "e").index(data[2][0]) # the first letter

    #     # animate the given philo image according to its activity
    #     QView.canvas.itemconfig(self._philo_img[i], image=self._act_img[j])

    #     # print a message to the text view
    #     QView.print_text("%010d Philo %1d is %s"%(data[0], i, data[2]))
 
#=============================================================================
QView.customize(BMS()) # set the QView customization
