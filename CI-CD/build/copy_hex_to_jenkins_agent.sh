#! /usr/bin/bash

# find generated hex file
# variable="$(command args ...)"

TARGET=$1

printf "Copying  flash hex image to Jenkins  ... Target is: ${TARGET}\n"

PATH_TO_HEX=0
if [ "$TARGET" == "PSOC63" ]; then
  PATH_TO_HEX=$(find ./BMS_PSOC63/build/ -type f -name 'bms_mcu*.hex')
elif [ "$TARGET" == "QN9080" ]; then
  PATH_TO_HEX=$(find ./QN908x/build-debug/build-version/ -type f -name 'bms_mcu*.hex')
else
  printf "Undefined build config\n"
  exit 1
fi
echo $PATH_TO_HEX

# copy the file to the remote jenkins-agent
scp -P 22 $PATH_TO_HEX jenkins@RPI4B.local:/home/Jenkins/jenkins-agent/bms_build_dir