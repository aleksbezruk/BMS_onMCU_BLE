#!/bin/bash
printf "=== Closing QSPY process...\n"
fuser -k -n udp 7701  # Exit QSPY & close UDP port
sleep 7
printf "=== QSPY process closed\n"
exit 0