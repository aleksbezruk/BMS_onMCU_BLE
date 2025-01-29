#!/bin/bash
sh "./Integration_Tests/start_qspy.sh" &
sh "./Integration_Tests/run_ITs.sh"
wait
exit 0