import simplepyble
import pytest
import time
from testfixture_general import resetDUT

pytest.ADAPTER = {}
pytest.BMS = {}

@pytest.mark.dependency(name="test_open_adapter")
def test_open_adapter(device_type):
    print("-------- test_open_adapter ------------")
    adapters = simplepyble.Adapter.get_adapters()
    for i, adapter in enumerate(adapters):
        print(f"{i}: {adapter.identifier()} [{adapter.address()}]")
    assert len(adapters) != 0, "No adapters found"
    choice = 0
    pytest.ADAPTER = adapters[choice]
    print(f"Selected adapter: {pytest.ADAPTER.identifier()} [{pytest.ADAPTER.address()}]")
    print("Preparing to test. It may takes up to 1 minute ...")
    resetDUT(device_type)
    time.sleep(3)  # for synchronization purpose
    pytest.ADAPTER.set_callback_on_scan_start(lambda: print("Scan started."))
    pytest.ADAPTER.set_callback_on_scan_stop(lambda: print("Scan complete."))
    pytest.ADAPTER.set_callback_on_scan_found(lambda peripheral: print(f"Found {peripheral.identifier()} [{peripheral.address()}]"))

@pytest.mark.dependency(depends=["test_open_adapter"], name="test_find_bms")
def test_find_bms(device_name):
    print("-------- test_find_bms ------------")
    print(f"Searching for device: {device_name}")
    try: 
        # Scan for 25 seconds
        pytest.ADAPTER.scan_for(25000)
        peripherals = pytest.ADAPTER.scan_get_results()
        is_bms_found = False
        for peripheral in peripherals:
            if peripheral.identifier() == device_name:
                is_bms_found = True
                pytest.BMS = peripheral
        assert is_bms_found == True, f"No {device_name} found"
    except:
        print("Retry scan")
        pytest.ADAPTER.scan_for(25000)
        peripherals = pytest.ADAPTER.scan_get_results()
        is_bms_found = False
        for peripheral in peripherals:
            if peripheral.identifier() == device_name:
                is_bms_found = True
                pytest.BMS = peripheral
        assert is_bms_found == True, f"No {device_name} found"

@pytest.mark.dependency(depends=["test_find_bms"], name="test_connect_bms")
def test_connect_bms():
    print("-------- test_connect_bms ------------")
    print(f"Connecting to: {pytest.BMS.identifier()} [{pytest.BMS.address()}]")
    try:
        pytest.BMS.connect()
    except:
        print("Try to reconnect")
        pytest.BMS.connect()    #retry, looks like SimpleBLE lib in some cases Fails to connect for unknown reason
    assert pytest.BMS.is_connected() == True, "BLE connect with BMS isn't established"
    print("-------- test_discover_bms ------------")
    time.sleep(3)
    services = pytest.BMS.services()
    is_BAS_char_discovered = False
    is_AIOS_server_discovered = False
    for service in services:
        print(f"Service: {service.uuid()}")
        if service.uuid() == "00001815-0000-1000-8000-00805f9b34fb":
            is_AIOS_server_discovered = True
        for characteristic in service.characteristics():
            print(f"    Characteristic: {characteristic.uuid()}")
            if characteristic.uuid() == "00002a19-0000-1000-8000-00805f9b34fb":
                is_BAS_char_discovered = True

            capabilities = " ".join(characteristic.capabilities())
            print(f"    Capabilities: {capabilities}")
    assert is_BAS_char_discovered == True, "BAS Battery Level Characteristic isn't discovered"
    assert is_AIOS_server_discovered == True, "Automation IO Service isn't discovered"
    print("-------- test_disconnect_bms ------------")
    time.sleep(3)
    pytest.BMS.disconnect()
    assert pytest.BMS.is_connected() == False, "BLE disconnect failed"
    print("Successfully disconnected.")
    time.sleep(3) # small sleep for synchronization purpose

# END OF FILE
