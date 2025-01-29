#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'

cd ./Integration_Tests

# Start QSPY
# sh "./start_qspy.sh" &
# sleep 5

# Run tests suit
pytest --version
pyocd reset --target cy8c6xx7_nosmif --uid 1714186503068400 # reset/boot target before run tests

pytest -s ./test_BLE_scan.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}The test_BLE_scan.py PASSED\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}The test_BLE_scan.py FAILED and returned the code $RETURN\n"
  pkill qspy
  fuser -k -n udp 7701  # close UDP port
  exit $RETURN
fi

pytest -s ./test_BLE_connect.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}==================================================\n"
  printf "${GREEN}The test_BLE_connect.py PASSED\n"
  printf "${GREEN}==================================================\n"
else
  printf "${RED}The test_BLE_connect.py FAILED and returned the code $RETURN\n"
  pkill qspy
  fuser -k -n udp 7701  # close UDP port
  exit $RETURN
fi

pytest -s ./test_BLE_BAS.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}The test_BLE_BAS.py PASSED\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}The test_BLE_BAS.py FAILED and returned the code $RETURN\n"
  pkill qspy
  fuser -k -n udp 7701  # close UDP port
  exit $RETURN
fi

pytest -s ./test_BLE_AIOS.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}================================================\n"
  printf "${GREEN}The test_BLE_AIOS.py PASSED\n"
  printf "${GREEN}================================================\n"
else
  printf "${RED}The test_BLE_AIOS.py FAILED and returned the code $RETURN\n"
  pkill qspy
  fuser -k -n udp 7701  # close UDP port
  exit $RETURN
fi

printf "${GREEN}===================================\n"
printf "${GREEN}===================================\n"
printf "${GREEN}All tests PASSED\n"
printf "${GREEN}===================================\n"
printf "${GREEN}===================================\n"

# Stop QSPY logging
pkill qspy
fuser -k -n udp 7701  # close UDP port
sleep 5

# return back to working directory after tests
cd ../
exit 0