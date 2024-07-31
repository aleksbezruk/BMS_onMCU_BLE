# test-script for QUTest unit testing harness
# see https://www.state-machine.com/qtools/qutest.html

def on_reset():
    print("---Reset target---")
    #expect_pause()
    #glb_filter(GRP_ALL)
    #continue_test()

# tests...
test(''' mainTask_ test ''')
#command(1, "mainTask_")
command(1, 0x10007B65)
expect("@timestamp UTEST mainTask_")
expect("@timestamp Trg-Done QS_RX_COMMAND")