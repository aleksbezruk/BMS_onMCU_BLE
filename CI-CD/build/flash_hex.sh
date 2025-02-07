#! /usr/bin/bash
pwd

# OpenOCD isn't used for the project, PyOCD is used because it's crosspltaform and easy to use
# openocd/bin/openocd -s openocd/scripts/ -f ./openocd_gdb/openocd.tcl -c "set DISABLE_SMIF 1" -c "set remotetimeout 25; monitor reset init; monitor psoc6 sflash_restrictions 1; monitor erase_all" -c "quit"

# Find the latest one hex file
HEX_FILE=0
HEX_FILE=$(ls ../../bms_build_dir/ -Art | tail -n 1)
printf "${GREEN}Flashing ../../bms_build_dir/${HEX_FILE} ... \n"

# Flash found hex file to the target
PATH=$PATH:$HOME/.local/bin
pyocd --version # ping PtOCD

pyocd erase --chip --target cy8c6xx7_nosmif --uid 1714186503068400
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}Erase success\n"
else
  printf "${RED}Erase FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

pyocd load --target cy8c6xx7_nosmif --uid 1714186503068400 ../../bms_build_dir/${HEX_FILE}
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}Flash success\n"
else
  printf "${RED}Flash FAILED and returned the code $RETURN\n"
  exit $RETURN
fi
