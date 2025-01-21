import os
from git import Repo
import time

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
path_to_hex = "../../BMS_PSOC63/build/" + hex_name + ".hex"
print(path_to_hex)
os.rename('../../BMS_PSOC63/build/app_combined.hex', path_to_hex)