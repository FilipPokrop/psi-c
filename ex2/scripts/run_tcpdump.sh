#!/bin/sh

CONTAINER=$1
DATE=$(date +%y-%m-%dT%H:%M:%S)
REGEX="^[0-9]{2}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.pcap"
FILES_LIMIT=${FILES_LIMIT:=10}
DATA_DIR=log/${CONTAINER}


[ -d ${DATA_DIR} ] || mkdir ${DATA_DIR}
FILES=$(ls ${DATA_DIR} | grep -E ${REGEX} | wc -l)
if [ ${FILES} -ge ${FILES_LIMIT} ]; then
  FILES_TO_DELETE_COUNT=$((${FILES} - ${FILES_LIMIT} + 1))
  FILES_TO_DELETE=$(ls ${DATA_DIR} | grep -E ${REGEX} | sort | head -n ${FILES_TO_DELETE_COUNT}) 

  for i in ${FILES_TO_DELETE}; do
    rm ${DATA_DIR}/${i}
  done
fi
# touch  ${DATA_DIR}/${DATE}.pcap 
tcpdump -U -w ${DATA_DIR}/${DATE}.pcap &
