#! /usr/bin/bash
cd ../../BMS_PSOC63
pwd

make getlibs
make clean BUILD_CONFIG=Debug
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN} PASSED\n"
else
  printf "${RED}FAILED, returned the code $RETURN\n"
  exit $RETURN
fi

cd ../CI-CD/build

# End of FILE #