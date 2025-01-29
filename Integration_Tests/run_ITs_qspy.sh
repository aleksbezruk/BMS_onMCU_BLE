#!/bin/bash
sh "./Integration_Tests/start_qspy.sh" &
sh "./Integration_Tests/run_ITs.sh" &
process_id=$!
echo "PID: $process_id"
wait $process_id
echo "Tests Exit status: $?"

printf "=== Closing tests execution process...\n"
fuser -k -n udp 7701  # Exit QSPY & close UDP port
sleep 15
printf "=== Tests execution process closed\n"
exit 0