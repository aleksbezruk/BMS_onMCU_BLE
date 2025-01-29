#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'

cd ./Integration_Tests

sleep 30  # synchronization with QSPY

# Run tests suit
pytest --version
printf "=== Reboot DUT ===\n"
pyocd reset --target cy8c6xx7_nosmif --uid 1714186503068400 # reset/boot target before run tests

sleep 20  #delay between tests to obtain QSPY logs
pytest -s ./test_BLE_scan.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}The test_BLE_scan.py PASSED\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}The test_BLE_scan.py FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

sleep 20  #delay between tests to obtain QSPY logs
pytest -s ./test_BLE_connect.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}==================================================\n"
  printf "${GREEN}The test_BLE_connect.py PASSED\n"
  printf "${GREEN}==================================================\n"
else
  printf "${RED}The test_BLE_connect.py FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

sleep 20  #delay between tests to obtain QSPY logs
pytest -s ./test_BLE_BAS.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}The test_BLE_BAS.py PASSED\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}The test_BLE_BAS.py FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

sleep 20  #delay between tests to obtain QSPY logs
pytest -s ./test_BLE_AIOS.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}================================================\n"
  printf "${GREEN}The test_BLE_AIOS.py PASSED\n"
  printf "${GREEN}================================================\n"
else
  printf "${RED}The test_BLE_AIOS.py FAILED and returned the code $RETURN\n"
  exit $RETURN
fi
sleep 20  #delay to obtain QSPY logs before exit

printf "${GREEN}===================================\n"
printf "${GREEN}===================================\n"
printf "${GREEN}All tests PASSED\n"
printf "${GREEN}===================================\n"
printf "${GREEN}===================================\n"

# return back to working directory after tests
cd ../
exit 0