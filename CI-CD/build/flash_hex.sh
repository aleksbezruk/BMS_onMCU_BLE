#! /usr/bin/bash
pwd

# OpenOCD isn't used for the project, PyOCD is used because it's crosspltaform and easy to use
# openocd/bin/openocd -s openocd/scripts/ -f ./openocd_gdb/openocd.tcl -c "set DISABLE_SMIF 1" -c "set remotetimeout 25; monitor reset init; monitor psoc6 sflash_restrictions 1; monitor erase_all" -c "quit"

#pyocd flash ../../bms_build_dir/${HEX_FILE} -t QN9080C --uid 727460214
#pyocd load --target QN9080C --uid 727460214 ../../bms_build_dir/${HEX_FILE}
#pyocd reset -t QN9080C --uid 727460214
#export BMS_HEX_FILE_PATH=/home/oleksandr/Projects/BMS_MCU/BMS_onMCU_BLE/QN908x/build-debug/build-version/bms_mcu_0-6-0_4dea545c_2-26-2026.hex

# Find the latest one hex file
HEX_FILE=0
HEX_FILE=$(ls ../../bms_build_dir/ -Art | tail -n 1)
printf "${GREEN}Flashing ../../bms_build_dir/${HEX_FILE} ... \n"

# Flash found hex file to the target
PATH=$PATH:$HOME/.local/bin
pyocd --version # ping PtOCD

TARGET=$1

# Erase chip
if [ "$TARGET" == "PSOC63" ]; then
  pyocd erase --chip --target cy8c6xx7_nosmif --uid 1714186503068400
elif [ "$TARGET" == "QN9080" ]; then
  # pyocd erase --chip -t QN9080C --uid 727460214
  printf "...\n"
else
  printf "Undefined build config\n"
  exit 1
fi

# Check erase status
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}Erase success\n"
else
  printf "${RED}Erase FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

# Program firmware image
if [ "$TARGET" == "PSOC63" ]; then
  pyocd load --target cy8c6xx7_nosmif --uid 1714186503068400 ../../bms_build_dir/${HEX_FILE}
elif [ "$TARGET" == "QN9080" ]; then
  export BMS_HEX_FILE_PATH=../../bms_build_dir/${HEX_FILE}
  envsubst < ./CI-CD/build/flash_qn9080_template.jlink > ./CI-CD/build/flash_qn9080.jlink
  #~/IDEs/Segger/JLink_Linux_V792k_x86_64/JLinkExe -NoGui 1 -device QN9080C -if SWD -speed 100 -autoconnect 1 -CommanderScript ./flash_qn9080.jlink
  /opt/SEGGER/JLink/JLinkExe -NoGui 1 -device QN9080C -if SWD -speed 100 -autoconnect 1 -CommanderScript ./CI-CD/build/flash_qn9080.jlink
else
  printf "Undefined build config\n"
  exit 1
fi

# Check program status
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}Flash success\n"
else
  printf "${RED}Flash FAILED and returned the code $RETURN\n"
  exit $RETURN
fi
