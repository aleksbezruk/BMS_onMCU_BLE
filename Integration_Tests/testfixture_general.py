#!/usr/bin/env python3
import pytest
from pyocd.core.helpers import ConnectHelper
from pyocd.core.target import Target
from pyocd.debug.elf.symbols import ELFSymbolProvider

# Device configuration
DEVICE_CONFIGS = {
    "PSOC63": {
        "unique_id": "1714186503068400",
        "target_override": "cy8c6xx7_nosmif",
        "frequency": 1000000
    },
    "QN9080": {
        "unique_id": "727460214",  # Segger J-Link LPCXpresso V2
        "target_override": "cortex_m",  # Use generic Cortex-M target for QN9080
        "frequency": 1000000
    }
}

# reset DUT
def resetDUT(device_type="PSOC63"):
    """
    Reset the Device Under Test (DUT)
    
    Args:
        device_type (str): Type of device to reset. Options: "PSOC63", "QN9080"
                          Defaults to "PSOC63" for backward compatibility
    """
    print(f"-------- reboot DUT ({device_type}) ------------")
    
    if device_type not in DEVICE_CONFIGS:
        raise ValueError(f"Unsupported device type: {device_type}. Supported types: {list(DEVICE_CONFIGS.keys())}")
    
    config = DEVICE_CONFIGS[device_type]
    
    # Connect to the target with device-specific options
    session = ConnectHelper.session_with_chosen_probe(
        unique_id=config["unique_id"], 
        options={
            "frequency": config["frequency"], 
            "target_override": config["target_override"]
        }
    )
    
    with session:
        target = session.target
        # Reset and run
        target.reset()
        print(f"-------- {device_type} reset completed ------------")

# END OF FILE
