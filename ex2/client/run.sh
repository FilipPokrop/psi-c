#!/bin/sh

# CMD="./main"
[ -n "$CMD" ] || CMD="./main -s 2048 -w 0.5"

/scripts/run_tcpdump.sh client

# python main.py 
${CMD} 
STATUS=$?

sleep 0.3

exit $STATUS
