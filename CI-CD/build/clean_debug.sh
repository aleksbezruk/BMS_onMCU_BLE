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

if [ "$TARGET" == "PSOC63" ]; then
  printf "Clean PSOC63 build config\n"
  make getlibs
  make clean BUILD_CONFIG=Debug
elif [ "$TARGET" == "QN9080" ]; then
  printf "Clean QN9080 build config\n"
  rm -rf ./build-debug
  mkdir ./build-debug
  cmake -S . -B ./build-debug -DBUILD_CONFIG=Debug
  cmake --build ./build-debug --target clean
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

cd ../CI-CD/build

# End of FILE #