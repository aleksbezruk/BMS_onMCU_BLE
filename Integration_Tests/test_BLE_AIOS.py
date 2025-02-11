import simplepyble
import pytest
import time

pytest.ADAPTER = {}
pytest.BMS = {}
pytest.service_characteristic_pair = []
pytest.AIOS_NUM_ITER = 2

@pytest.mark.dependency(name="test_open_adapter")
def test_open_adapter():
    print("-------- test_open_adapter ------------")
    adapters = simplepyble.Adapter.get_adapters()
    for i, adapter in enumerate(adapters):
        print(f"{i}: {adapter.identifier()} [{adapter.address()}]")
    assert len(adapters) != 0, "No adapters found"
    choice = 0
    pytest.ADAPTER = adapters[choice]
    print(f"Selected adapter: {pytest.ADAPTER.identifier()} [{pytest.ADAPTER.address()}]")
    pytest.ADAPTER.set_callback_on_scan_start(lambda: print("Scan started."))
    pytest.ADAPTER.set_callback_on_scan_stop(lambda: print("Scan complete."))
    pytest.ADAPTER.set_callback_on_scan_found(lambda peripheral: print(f"Found {peripheral.identifier()} [{peripheral.address()}]"))

@pytest.mark.dependency(depends=["test_open_adapter"], name="test_find_bms")
@pytest.mark.repeat(2)
def test_find_bms():
    print("-------- test_find_bms ------------")
    # Scan for 15 seconds
    pytest.ADAPTER.scan_for(15000)
    peripherals = pytest.ADAPTER.scan_get_results()
    is_bms_found = False
    for peripheral in peripherals:
        if peripheral.identifier() == "BMS_MCU":
            is_bms_found = True
            pytest.BMS = peripheral
    assert is_bms_found == True, "No BMS found"

@pytest.mark.dependency(depends=["test_find_bms"], name="test_connect_bms")
def test_connect_bms():
    print("-------- test_connect_bms ------------")
    print(f"Connecting to: {pytest.BMS.identifier()} [{pytest.BMS.address()}]")
    pytest.BMS.connect()
    assert pytest.BMS.is_connected() == True, "BLE connect with BMS isn't established"

@pytest.mark.dependency(depends=["test_connect_bms"], name="test_discover_aios")
def test_discover_aios():
    print("-------- test_discover_aios ------------")
    services = pytest.BMS.services()
    is_AIOS_char_discovered = False
    for service in services:
        print(f"Service: {service.uuid()}")
        for characteristic in service.characteristics():
            print(f"    Characteristic: {characteristic.uuid()}")
            if characteristic.uuid() == "37af9ae2-211d-4436-9d26-3a9ed02efeea": # Digital IO char
                is_AIOS_char_discovered = True
                pytest.service_characteristic_pair.append((service.uuid(), characteristic.uuid()))
            capabilities = " ".join(characteristic.capabilities())
            print(f"    Capabilities: {capabilities}")
    assert is_AIOS_char_discovered == True, "Automation IO Service isn't discovered"

@pytest.mark.dependency(depends=["test_discover_aios"], name="test_read_switch_state")
@pytest.mark.repeat(2)
def test_read_switch_state():
    print("-------- test_read_switch_state ------------")
    service_uuid, characteristic_uuid = pytest.service_characteristic_pair[0]
    swData = pytest.BMS.read(service_uuid, characteristic_uuid)
    swState = swData[0]
    print("Switches state = %d" %(swState))
    assert swState == 0, "All switches should be disabled at start up"

@pytest.mark.dependency(depends=["test_read_switch_state"], name="test_enable_disable_switch")
def test_enable_disable_switch():
    print("-------- test_enable_disable_switch ------------")
    num_iter = 0
    while num_iter < pytest.AIOS_NUM_ITER:
        num_iter += 1
        print("AIOS switches test, iteration #%d"%(num_iter))
        # Write the content to the characteristic
        # Note: `write_request` required the payload to be presented as a bytes object.
        service_uuid, characteristic_uuid = pytest.service_characteristic_pair[0]

        # Wait notification response from DUT
        print("-------- test_enable_switches ------------")
        swState = []
        pytest.BMS.notify(service_uuid, characteristic_uuid, lambda data: swState.append(data[0]))
        bytes_array = str.encode("1000")
        pytest.BMS.write_request(service_uuid, characteristic_uuid, bytes_array)
        time.sleep(60)
        print("Switches state notif = %d" %(swState[0]))
        assert swState[0] == 0x01, "Discharge switch was not enabled"

        print("-------- test_disable_switches ------------")
        # Wait notification response from DUT
        swState = []
        pytest.BMS.notify(service_uuid, characteristic_uuid, lambda data: swState.append(data[0]))
        bytes_array = str.encode("0000")
        pytest.BMS.write_request(service_uuid, characteristic_uuid, bytes_array)
        time.sleep(60)
        print("Switches state notif = %d" %(swState[0]))
        assert swState[0] == 0x00, "Switches was not disabled"

@pytest.mark.dependency(depends=["test_enable_disable_switch"], name="test_disconnect_bms")
def test_disconnect_bms():
    print("-------- test_disconnect_bms ------------")
    pytest.BMS.disconnect()
    assert pytest.BMS.is_connected() == False, "BLE disconnect failed"
    print("Successfully disconnected.")
    print("Wait some time before shutdown test... It may takes up to 1 minute.")
    time.sleep(40) # for synchronization purpose

# END OF FILE
