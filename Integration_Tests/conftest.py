import pytest
import warnings

# Suppress specific deprecation warnings from third-party libraries
warnings.filterwarnings("ignore", message="is_resource is deprecated", category=DeprecationWarning)

def pytest_addoption(parser):
    parser.addoption(
        "--device-name",
        action="store",
        dest="device_name",
        default="BMS_MCU",
        help="BLE device name to search for (default: BMS_MCU)"
    )
    parser.addoption(
        "--device-type",
        action="store",
        dest="device_type",
        default="PSOC63",
        choices=["PSOC63", "QN9080"],
        help="Type of device to reset and test (default: PSOC63)"
    )

@pytest.fixture(scope="session")
def device_name(request):
    return request.config.getoption("device_name")

@pytest.fixture(scope="session")
def device_type(request):
    return request.config.getoption("device_type")
