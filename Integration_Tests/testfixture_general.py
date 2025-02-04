#!/usr/bin/env python3
import pytest
from pyocd.core.helpers import ConnectHelper
from pyocd.core.target import Target
from pyocd.debug.elf.symbols import ELFSymbolProvider

# reset DUT
def testfixture_resetDUT():
    print("-------- reboot DUT ------------")
    # Connect to the target with some options.
    session = ConnectHelper.session_with_chosen_probe(unique_id = "1714186503068400", options = {"frequency": 1000000, "target_override": "cy8c6xx7_nosmif"})
    with session:
        target = session.target
        # Reset and run.
        target.reset()

# END OF FILE
