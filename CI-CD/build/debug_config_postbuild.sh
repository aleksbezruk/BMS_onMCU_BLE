#! /usr/bin/bash
printf "===============================================\n"
printf "Debug config post build Job in progress ... \n"
printf "===============================================\n"

# pip3 --version
# pip3 install -r ../requirements.txt --break-system-packages
# RETURN=$?
# if [ $RETURN -eq 0 ];
# then
#   printf "${GREEN}===============================================\n"
#   printf "${GREEN}Python packages installed successfully \n"
#   printf "${GREEN}===============================================\n"
# else
#   printf "${RED}Installing python requirements FAILED and returned the code $RETURN\n"
#   exit $RETURN
# fi

cd build
ls -a

python3 --version
python3 ../../CI-CD/build/build_ver.py

ls -a   # show result out files
