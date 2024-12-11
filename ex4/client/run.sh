#!/bin/sh

# cmd="./main -s 8"
[ -n "$CMD" ] || CMD="./main"
# echo $PATH


/scripts/run_tcpdump.sh client 

${CMD} 
STATUS=$?

sleep 0.3

exit $STATUS
