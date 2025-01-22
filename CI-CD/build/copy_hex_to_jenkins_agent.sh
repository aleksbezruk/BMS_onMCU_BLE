#! /usr/bin/bash

# find generated hex file
# variable="$(command args ...)"
PATH_TO_HEX=0
PATH_TO_HEX=$(find ./BMS_PSOC63/build/ -type f -name 'bms_mcu*.hex')
echo $PATH_TO_HEX

# copy the file to the remote jenkins-agent
scp -P 22 $PATH_TO_HEX jenkins@RPI4B.local:/home/Jenkins/jenkins-agent/bms_build_dir