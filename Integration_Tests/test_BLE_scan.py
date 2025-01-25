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
    # Scan for 15 seconds
    pytest.ADAPTER.scan_for(15000)
    peripherals = pytest.ADAPTER.scan_get_results()
    is_bms_found = False
    for peripheral in peripherals:
        if peripheral.identifier() == "BMS_MCU":
            is_bms_found = True
            pytest.BMS = peripheral
    assert is_bms_found == True, "No BMS found"

@pytest.mark.dependency(depends=["test_find_bms"], name="test_advertising_data")
def test_advertising_data():
    print("-------- test_advertising_data ------------")
    connectable_str = "Connectable" if pytest.BMS.is_connectable() else "Non-Connectable"
    assert connectable_str == "Connectable", "BMS isn't Connectable"
    services = pytest.BMS.services()
    is_BAS_found = False
    is_AIOS_found = False
    for service in services:
        print(f"    Service UUID: {service.uuid()}")
        print(f"    Service data: {service.data()}")
        if service.uuid() == "0000180f-0000-1000-8000-00805f9b34fb":
            is_BAS_found = True
        if service.uuid() == "00001815-0000-1000-8000-00805f9b34fb":
            is_AIOS_found = True
    assert is_BAS_found == True, "BAS not found in Advertising data"
    assert is_AIOS_found == True, "AIOS not found in Advertising data"
