#!/bin/bash

pytest -s ./test_BLE_scan.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  echo "The test_BLE_scan.py was executed successfuly"
else
  echo "The test_BLE_scan.py was NOT executed successfuly and returned the code $RETURN"
  exit $RETURN
fi

pytest -s ./test_BLE_connect.py
RETURN=$?
if [ $RETURN -eq 0 ];
then
  echo "The test_BLE_connect.py was executed successfuly"
else
  echo "The test_BLE_connect.py was NOT executed successfuly and returned the code $RETURN"
  exit $RETURN
fi

exit 0