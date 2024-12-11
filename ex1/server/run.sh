#!/bin/sh

[ -n "$CMD" ] || CMD="./main"

/scripts/run_tcpdump.sh server 

${CMD} 
STATUS=$?

sleep 0.3

exit $STATUS
