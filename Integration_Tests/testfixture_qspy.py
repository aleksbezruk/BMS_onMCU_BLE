import socket
import pytest
import time
from struct import pack
import struct
import ctypes
import datetime

pytest._sock = {}
pytest._local_port = 7701
pytest._host_addr = ["localhost", 7701]

pytest._QSPY_ATTACH     = 128
pytest._QSPY_DETACH     = 129
pytest._PKT_ATTACH_CONF = 128
pytest._PKT_DETACH      = 129
pytest._TRGT_RESET      = 2
pytest._TRGT_COMMAND    = 1

pytest._qspy_tx_seq = 0

# QSPY records IDs
pytest.QS_USER0         = 100
pytest.QS_MAIN          = pytest.QS_USER0
pytest.QS_UTEST         = pytest.QS_USER0 + 1
pytest.QS_BSP           = pytest.QS_USER0 + 2
pytest.QS_ADC           = pytest.QS_USER0 + 3
pytest.QS_BLE_TRACE     = pytest.QS_USER0 + 4
pytest.QS_BLE_BAS       = pytest.QS_USER0 + 5
pytest.QS_BLE_AIOS      = pytest.QS_USER0 + 6

# QSPY spare data/constants
pytest.qs_fmt = "xBHxLxxxQ"
pytest.qs_size_objPtr   = 4
pytest.qs_size_funPtr   = 4
pytest.qs_size_tstamp   = 4
pytest.qs_size_sig      = 2

################################################
#### QSPY low level functions
################################################

# see qview.py for reference how to communicate with QSPY running process on UDP socket:
# 1. def _poll():
# 2. def _sendTo(packet, str=None):
# 3. def customize(cust):
# 4. _host_addr
# 5. QSpy._attach()
# 6. QSpy._detach()

# Attach QSPY
# poll the UDP socket until the QSpy confirms ATTACH
def QSPY_attach():
    _sendTo(pack("<BB", pytest._QSPY_ATTACH , 0x1))
    qspy_attach_ctr  = 50
    # start poll the UDP socket (see QSPY._poll0() for reference)
    while qspy_attach_ctr > 0:
        try:
            packet = pytest._sock.recv(4096)
            if not packet:
                raise Exception("--QSPY attach: UDP Socket Error",
                    "Connection closed by QSpy")
        except OSError: # non-blocking socket...
            continue # <======== most frequent return (no packet) to the next iteration/retry
        except Exception:
            raise Exception("--QSPY attach: UDP Socket Error",
               "Uknown UDP socket error")

        # parse the packet...
        dlen = len(packet)
        if dlen < 2:
            raise Exception("--QSPY attach: Communication Error",
               "UDP packet from QSpy too short")

        recID = packet[1]
        if recID == pytest._PKT_ATTACH_CONF:
            # send either reset or target-info request
            # (keep the poll0 loop running)
            print("--QSPY attach: success. Send cmd to reset target")
            _sendTo(pack("<B", pytest._TRGT_RESET), enWakeup = True)
            # else:
            #     QSpy._sendTo(pack("<B", QSpy._TRGT_INFO))
            return

        elif recID == pytest._PKT_DETACH:
            raise Exception("--QSPY attach: unexpected QSPY detach")

        qspy_attach_ctr -= 1

    raise Exception("--QSPY attach: exceed number of retries")
    

# Detach QSPY
def QSPY_detach():
    if pytest._sock is None:
        return
    _sendTo(pack("<B", pytest._QSPY_DETACH))
    time.sleep(0.25) # let the socket finish sending the packet
    #QSpy._sock.shutdown(socket.SHUT_RDWR)
    pytest._sock.close()
    pytest._sock = None

# Init QSPY
# getent hosts "$(hostname)" | awk '{ print $1 }'
def QSPY_init():
    # Create socket
    pytest._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    pytest._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    pytest._sock.setblocking(0) # NON-BLOCKING socket
    try:
        #print("try to bind to Port:" "%d"%(pytest._local_port))
        #pytest._sock.bind(('localhost', pytest._local_port)
        pytest._sock.bind(("0.0.0.0", 0))

    except Exception:
        raise Exception("UDP Socket Error",
            "Can't bind the UDP socket\n"
            "to the specified local_host.\n"
            "Check if other instances of qspyview\n"
            "or qutest are running...")
    print("Success bind to QSPY.")
    QSPY_attach()

# De-init QSPY
# See QSpy._detach() for reference
def QSPY_deinit():
    QSPY_detach()

# Send packet to QSPY via socket
def _sendTo(packet, enWakeup = False, str=None):
    print("_tx_seq: " "%d"%(pytest._qspy_tx_seq))
    tx_packet = bytearray([pytest._qspy_tx_seq & 0xFF])
    tx_packet.extend(packet)
    if str is not None:
        tx_packet.extend(bytes(str, "utf-8"))
        tx_packet.extend(b"\0") # zero-terminate
    try:
        host_addr = tuple(pytest._host_addr) # convert to immutable tuple
        if enWakeup == True:
            print("Send Traget wakeup")
            wakeUp_packet = pack("<BBIII", pytest._TRGT_COMMAND,
                                    255, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA)
            tx_wakeUp_packet = bytearray([pytest._qspy_tx_seq & 0xFF])
            tx_wakeUp_packet.extend(wakeUp_packet)
            pytest._sock.sendto(tx_wakeUp_packet, host_addr)
            pytest._qspy_tx_seq += 1
        else:
            print("No Traget wakeup")
        print("_sock.sendto: " "%s, %d"%(pytest._host_addr[0], pytest._host_addr[1]))
        print("tx_packet: " "%s"%(tx_packet))
        pytest._sock.sendto(tx_packet, host_addr)
    except Exception:
        raise Exception("--QSPY _sendTo: UDP Socket Error")
    pytest._qspy_tx_seq += 1

# Get a QSPY packet on UDP socket
# return: packet list OR 0 - if no packet at this time
def QSPY_getPacket():
    try:
        packet = pytest._sock.recv(4096)
        if not packet:
            raise Exception("--QSPY _getPacket: UDP Socket Error",
                                "Connection closed by QSpy")
    except OSError: # non-blocking socket...
        return 0 # <============= no packet at this time
    except Exception:
        raise Exception("--QSPY _getPacket: UDP Socket Error",
                            "Uknown UDP socket error")

    # parse the packet...
    dlen = len(packet)
    if dlen < 2:
        raise Exception("--QSPY _getPacket: UDP Socket Data Error",
                            "UDP packet from QSpy too short")

    return packet   # UDP socket packet is valid at this point

################################################
#### QSPY Utils / auxiliary functions
################################################
## @brief Unpack a QS trace record
#
# @description
# The qunpack() facility is similar to Python `struct.unpack()`,
# specifically designed for unpacking binary QP/Spy packets.
# qunpack() handles all data formats supported by struct.unpack()`,
# plus data formats specific to QP/Spy. The main benefit of qunpack()
# is that it automatically applies the Target-supplied info about
# various the sizes of various elements, such as Target timestamp,
# Target object-pointer, Target event-signal, zero-terminated string, etc.
## @brief pokes data into the Target
# @sa qutest_dsl.poke()
#
# @param[in] fmt  format string
# @param[in] bstr byte-string to unpack
#
# @returns
# The result is a tuple with elements corresponding to the format items.
#
# The additional format characters have the following meaning:
#
# - T : QP/Spy timestamp -> integer, 2..4-bytes (Target dependent)
# - O : QP/Spy object pointer -> integer, 2..8-bytes (Target dependent)
# - F : QP/Spy function pointer -> integer, 2..8-bytes (Target dependent)
# - S : QP/Spy event signal -> integer, 1..4-bytes (Target dependent)
# - Z : QP/Spy zero-terminated string -> string of n-bytes (variable length)
#
# @usage
# @include qunpack.py
#
def qunpack(fmt, bstr):
    n = 0
    m = len(fmt)
    bord = "<" # default little-endian byte order
    if fmt[0:1] in ("@", "=", "<", ">", "!"):
        bord = fmt[0:1]
        n += 1
    data = []
    offset = 0
    while n < m:
        fmt1 = fmt[n:(n+1)]
        u = ()
        if fmt1 in ("B", "b", "c", "x", "?"):
            u = struct.unpack_from(bord + fmt1, bstr, offset)
            offset += 1
        elif fmt1 in ("H", "h"):
            u = struct.unpack_from(bord + fmt1, bstr, offset)
            offset += 2
        elif fmt1 in ("I", "L", "i", "l", "f"):
            u = struct.unpack_from(bord + fmt1, bstr, offset)
            offset += 4
        elif fmt1 in ("Q", "q", "d"):
            u = struct.unpack_from(bord + fmt1, bstr, offset)
            offset += 8
        elif fmt1 == "T":
            u = struct.unpack_from(bord + pytest.qs_fmt[pytest.qs_size_tstamp],
                                   bstr, offset)
            offset += pytest.qs_size_tstamp
        elif fmt1 == "O":
            u = struct.unpack_from(bord + pytest.qs_fmt[pytest.qs_size_objPtr],
                                   bstr, offset)
            offset += pytest.qs_size_objPtr
        elif fmt1 == "F":
            u = struct.unpack_from(bord + pytest.qs_fmt[pytest.qs_size_funPtr],
                                   bstr, offset)
            offset += pytest.qs_size_funPtr
        elif fmt1 == "S":
            u = struct.unpack_from(bord + pytest.qs_fmt[pytest.qs_size_sig],
                                  bstr, offset)
            offset += pytest.qs_size_sig
        elif fmt1 == "Z": # zero-terminated C-string
            end = offset
            while bstr[end]: # not zero-terminator?
                end += 1
            u = (bstr[offset:end].decode(),)
            offset = end + 1 # inclue the terminating zero
        else:
            assert 0, "qunpack(): unknown format"
        data.extend(u)
        n += 1
    return tuple(data)

################################################
#### QSPY User APIs
################################################

# Get ADC measurement result
# timeout - in seconds
# retuens adc_meas_result, @ref adc_meas_result
def QSPY_get_ADC_meas(timeout=40):
    adc_meas_timeout = timeout
    polling_step = 0.25 #seconds
    num_iterations = int(adc_meas_timeout / polling_step)
    # start poll the UDP socket (see QSPY._poll0() for reference)
    while num_iterations > 0:
        time.sleep(polling_step)
        packet = QSPY_getPacket()
        if packet == 0:
            continue    # no packet yet

        # parse packet
        recID = packet[1]
        if recID == pytest._PKT_DETACH:
            raise Exception("--QSPY attach: unexpected QSPY detach")

        elif recID <= 124: # other binary data
            # intercept the QS_USER_00 application-specific packet(s) from Main task
            # this packet has the following structure:
            # record-ID, seq-num, Timestamp, zero terminated message string, switches state
            # check ADC measurement results
            if recID == pytest.QS_MAIN:
                # unpack: Timestamp->data[0], msg_string->data[1], switches_state->data[2]
                data = qunpack("xxTxZ", packet)
                msg_string = data[1]
                # filter messages received from MAIN task
                if msg_string == "Banks volt: ":
                    # packet structure: record-ID, seq-num, Timestamp, msg string, banks volt with format bytes
                    # unpack: Timestamp->tmp_data[0], msg_string->tmp_data[1], bank1->tmp_data[3], ... bank4->tmp_data[9]
                    #         fullVbat->tmp_data[11]
                    tmp_data = qunpack("xxTZBHBHBHBHBH", packet)
                    target_timestamp = tmp_data[0]
                    b1 = ctypes.c_short(tmp_data[3]).value  # int16_t
                    b2 = ctypes.c_short(tmp_data[5]).value
                    b3 = ctypes.c_short(tmp_data[7]).value
                    b4 = ctypes.c_short(tmp_data[9]).value
                    fullVbat = ctypes.c_short(tmp_data[11]).value
                    # print a message to the Console
                    now = datetime.datetime.now()
                    taregt_timestamp_str = "%010d:"%(target_timestamp)
                    string =  str(now.time()) + " [" + taregt_timestamp_str.replace(":", "") + "] " + "banks voltage in mV: b1=%5d, b2=%5d, b3=%5d, b4=%5d fullBat=%5d"%(b1, b2, b3, b4, fullVbat)
                    print(string)
                    adc_meas_result = {
                        "bank1": b1,
                        "bank2": b2,
                        "bank3": b3,
                        "bank4": b4,
                        "fullVbat": fullVbat
                    }
                    return adc_meas_result

        num_iterations -= 1

    raise Exception("--QSPY _get_ADC_meas: receive ADC measurement timeout")

# END OF FILE
