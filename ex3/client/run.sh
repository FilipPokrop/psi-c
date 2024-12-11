#!/bin/sh

# cmd="./main -s 8"
[ -n "$CMD" ] || CMD="./main -c 4 www.google.pl"
# echo $PATH


/scripts/run_tcpdump.sh client 
# sleep 5
${CMD} 
STATUS=$?

sleep 0.3

exit $STATUS
