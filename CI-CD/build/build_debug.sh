#! /usr/bin/bash
TARGET=$1

if [ "$TARGET" == "PSOC63" ]; then
  cd ../../BMS_PSOC63  
elif [ "$TARGET" == "QN9080" ]; then
  cd ../../QN908x
else
  printf "Undefined build config\n"
  exit 1
fi

pwd

#/opt/Tools/ModusToolbox/tools_3.2/lcs-manager-cli/lcs-manager-cli --update-existing
if [ "$TARGET" == "PSOC63" ]; then
  printf "Build PDOC63 target... \n"
  make getlibs
  make -j4 build --output-sync BUILD_CONFIG=Debug
elif [ "$TARGET" == "QN9080" ]; then
  printf "Build QN9080 target... \n"
  #cmake -S . -B ./build-debug -DBUILD_CONFIG=Debug
  rm -rf ./build-debug 
  mkdir -p ./build-debug
  cd ./build-debug
  cmake -S ../ -DBUILD_CONFIG=Debug
  cmake --build . 2>&1 | tee build.log; echo 'Error count:'; grep -i 'error:' build.log | wc -l; echo 'Warning count:'; grep -i 'warning:' build.log | wc -l
else
  printf "Undefined build config\n"
  exit 1
fi

# Check status
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN} PASSED\n"
else
  printf "${RED}FAILED, returned the code $RETURN\n"
  exit $RETURN
fi

# return to directory that contains 'build' scripts
if [ "$TARGET" == "PSOC63" ]; then
  cd ../CI-CD/build
elif [ "$TARGET" == "QN9080" ]; then
  cd ../../CI-CD/build
else
  printf "Undefined build config\n"
  exit 1
fi

# End of FILE #