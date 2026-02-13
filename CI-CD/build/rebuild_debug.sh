#! /usr/bin/bash
cd ./CI-CD/build

GREEN='\033[0;32m'
RED='\033[0;31m'

TARGET=$1
printf "Rebuild... Target is: ${TARGET}\n"

printf "${GREEN}===============================================\n"
printf "${GREEN}Cleaning Debug build configuration ... \n"
printf "${GREEN}===============================================\n"
./clean_debug.sh ${TARGET}
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}Clean Debug build success\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}Clean Debug configuration FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

printf "${GREEN}===============================================\n"
printf "${GREEN}Building Debug configuration ... \n"
printf "${GREEN}===============================================\n"
./build_debug.sh ${TARGET}
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}Build Debug cobfiguration success\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}Build Debug cobfiguration FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

printf "${GREEN}===============================================\n"
printf "${GREEN}Debug config postbuild job in progress ... \n"
printf "${GREEN}===============================================\n"
cd ../../BMS_PSOC63
../CI-CD/build/debug_config_postbuild.sh ${TARGET}
RETURN=$?
if [ $RETURN -eq 0 ];
then
  printf "${GREEN}===============================================\n"
  printf "${GREEN}Debug config postbuild job success\n"
  printf "${GREEN}===============================================\n"
else
  printf "${RED}Debug config postbuild job FAILED and returned the code $RETURN\n"
  exit $RETURN
fi

cd ../
printf "${GREEN}Rebuild Debug completed\n"

# End of FILE #