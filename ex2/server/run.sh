#!/bin/sh

[ -n "$CMD" ] || CMD="./main -s 512 -w 1"

/scripts/run_tcpdump.sh server 

${CMD} 
STATUS=$?

sleep 0.3

exit $STATUS
