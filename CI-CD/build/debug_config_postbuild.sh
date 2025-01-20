#! /usr/bin/bash
printf "===============================================\n"
printf "Debug config post build Job in progress ... \n"
printf "===============================================\n"

cd build
ls -a

python3 --version
python3 ../../CI-CD/build/build_ver.py

ls -a   # show result out files
