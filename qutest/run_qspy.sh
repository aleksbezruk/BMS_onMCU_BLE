export QTOOLS=~/QuantumLeaps/qp-linux_7.3.4/qp/qtools
# -c /dev/ttyACM0
echo "Try to open com port: $1"
./qspy -u 7701 -c /dev/$1 -b 115200 -d
