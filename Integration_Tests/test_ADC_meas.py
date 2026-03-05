import pytest
import time

from testfixture_qspy import QSPY_init
from testfixture_qspy import QSPY_deinit
from testfixture_qspy import QSPY_get_ADC_meas

#####################
# Constants
#####################
pytest.ADC_MEAS_NUM_ITERATIONS = 2

@pytest.mark.dependency(name="test_open_qspy")
def test_open_qspy():
    print("-------- test_open_qspy ------------")
    QSPY_init()
    assert bool(pytest._sock) == True, "QSPY socket isn't initialized"

@pytest.mark.dependency(depends=["test_open_qspy"], name="test_receive_periodic_adc")
def test_receive_periodic_adc():
    print("-------- test_receive_periodic_adc ------------")
    num_iter = pytest.ADC_MEAS_NUM_ITERATIONS
    while num_iter > 0:
        print("Iteration #", "%d"%(pytest.ADC_MEAS_NUM_ITERATIONS - num_iter + 1))
        adc_result = QSPY_get_ADC_meas()
        assert (adc_result["bank1"] >= 2700 and adc_result["bank1"] < 4300), "Bank1 voltage out of range"
        assert (adc_result["bank2"] >= 2700 and adc_result["bank2"] < 4300), "Bank2 voltage out of range"
        assert (adc_result["bank3"] >= 2700 and adc_result["bank3"] < 4300), "Bank3 voltage out of range"
        assert (adc_result["bank4"] >= 2700 and adc_result["bank4"] < 4300), "Bank4 voltage out of range"
        assert (adc_result["fullVbat"] >= 10800 and adc_result["fullVbat"] < 17000), "fullVbat voltage out of range"

        num_iter -= 1

@pytest.mark.dependency(depends=["test_receive_periodic_adc"], name="test_close_qspy")
def test_close_qspy():
    print("-------- test_close_qspy ------------")
    time.sleep(5)
    QSPY_deinit()
    assert pytest._sock == None, "close QSPY socket Failed"

# END OF FILE
