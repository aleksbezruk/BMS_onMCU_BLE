#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'

cd ./Integration_Tests

sleep 10  # synchronization with QSPY

# Run tests suit for QN9080 device
pytest --version
printf "=== Reboot QN9080 DUT ===\n"
pyocd reset --target cortex_m --uid 727460214 # reset/boot QN9080 target before run tests

sleep 5  #delay between tests to obtain QSPY logs

pytest -s ./test_ADC_meas.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}The test_ADC_meas.py PASSED (QN9080)\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}The test_ADC_meas.py FAILED and returned the code $RETURN (QN9080)\n"
  exit $RETURN
fi
sleep 5  #delay between tests to obtain QSPY logs

pytest -s ./test_BLE_scan.py --device-name "QN9080_BMS" --device-type "QN9080"
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}The test_BLE_scan.py PASSED (QN9080)\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}The test_BLE_scan.py FAILED and returned the code $RETURN (QN9080)\n"
  exit $RETURN
fi

sleep 5  #delay between tests to obtain QSPY logs
pytest -s ./test_BLE_connect.py --device-name "QN9080_BMS" --device-type "QN9080"
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}==================================================\n"
  printf "${GREEN}The test_BLE_connect.py PASSED (QN9080)\n"
  printf "${GREEN}==================================================\n"
else
  printf "${RED}The test_BLE_connect.py FAILED and returned the code $RETURN (QN9080)\n"
  exit $RETURN
fi

sleep 5  #delay between tests to obtain QSPY logs
pytest -s ./test_BLE_BAS.py --device-name "QN9080_BMS" --device-type "QN9080"
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}The test_BLE_BAS.py PASSED (QN9080)\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}The test_BLE_BAS.py FAILED and returned the code $RETURN (QN9080)\n"
  exit $RETURN
fi

sleep 5  #delay between tests to obtain QSPY logs
pytest -s ./test_BLE_AIOS.py --device-name "QN9080_BMS" --device-type "QN9080"
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}================================================\n"
  printf "${GREEN}The test_BLE_AIOS.py PASSED (QN9080)\n"
  printf "${GREEN}================================================\n"
else
  printf "${RED}The test_BLE_AIOS.py FAILED and returned the code $RETURN (QN9080)\n"
  exit $RETURN
fi
sleep 30  #delay to obtain QSPY logs before exit

printf "${GREEN}===================================\n"
printf "${GREEN}===================================\n"
printf "${GREEN}All QN9080 tests PASSED\n"
printf "${GREEN}===================================\n"
printf "${GREEN}===================================\n"

# return back to working directory after tests
cd ../
exit 0
