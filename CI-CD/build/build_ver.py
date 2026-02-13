import os
from git import Repo
import time
import sys
from pathlib import Path
import shutil

def get_version_number():
    version_file = [f for f in os.listdir("../../") if "version.txt" in f]
    print(version_file)
    if len(version_file) != 1:
        raise ValueError('should be only one dictionary file in the current directory')
    filename = version_file[0]
    filePath = "../../" + filename

    file1 = open(filePath, 'r')
    Lines = file1.readlines()
    return Lines[0]

# rorepo is a Repo instance pointing to the git-python repository.
# For all you know, the first argument to Repo is a path to the repository you
# want to work with.
target = sys.argv[1]
print("Add buuld version for target: " + target) 

repo = Repo("../../")
assert not repo.bare
headcommit = repo.head.commit
short_commit_hash = headcommit.hexsha[0:8]
print(short_commit_hash)
UTC_time = time.gmtime(headcommit.committed_date)
build_ver = short_commit_hash + "_" + str(UTC_time.tm_mon) + "-" + str(UTC_time.tm_mday) + "-" + str(UTC_time.tm_year)
ver_num = get_version_number()
ver_num_mod = ver_num.replace(".", "-")
print(ver_num_mod)

hex_name = "bms_mcu" + "_" + ver_num_mod + "_" + build_ver
if target == "PSOC63":
    path_to_hex = "../../BMS_PSOC63/build/" + hex_name + ".hex"
    os.rename('../../BMS_PSOC63/build/app_combined.hex', path_to_hex)
elif target == "QN9080":
    build_dir = Path("../../QN908x/build-debug/build-version")
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir()
    shutil.copy("../../QN908x/build-debug/BMS_QN9080_Debug.hex", "../../QN908x/build-debug/build-version/")
    path_to_hex = "../../QN908x/build-debug/build-version/" + hex_name + ".hex"
    os.rename('../../QN908x/build-debug/build-version/BMS_QN9080_Debug.hex', path_to_hex)
else:
    raise Exception("Undefined target")

print(path_to_hex)
