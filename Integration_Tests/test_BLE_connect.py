import simplepyble
import pytest

pytest.ADAPTER = {}
pytest.BMS = {}

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

@pytest.mark.dependency(depends=["test_open_adapter"], name="test_find_bms")
def test_find_bms():
    print("-------- test_find_bms ------------")
    pytest.ADAPTER.set_callback_on_scan_start(lambda: print("Scan started."))
    pytest.ADAPTER.set_callback_on_scan_stop(lambda: print("Scan complete."))
    pytest.ADAPTER.set_callback_on_scan_found(lambda peripheral: print(f"Found {peripheral.identifier()} [{peripheral.address()}]"))
    # Scan for 5 seconds
    pytest.ADAPTER.scan_for(5000)
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

@pytest.mark.dependency(depends=["test_connect_bms"], name="test_discover_bms")
def test_discover_bms():
    print("-------- test_discover_bms ------------")
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

@pytest.mark.dependency(depends=["test_discover_bms"], name="test_disconnect_bms")
def test_disconnect_bms():
    print("-------- test_disconnect_bms ------------")
    pytest.BMS.disconnect()
    assert pytest.BMS.is_connected() == False, "BLE disconnect failed"
    print("Successfully disconnected.")
