#!/bin/bash
COUNTER=0
while [ $COUNTER -lt 100  ]; do
i=$COUNTER
echo $i
curl -X POST -d "id=$i&name=$i" http://127.0.0.1:6502/api/user/registered | jq .
curl -X POST -d "id=$i&name=$i" http://127.0.0.1:6502/api/user/connected | jq .
let COUNTER=COUNTER+1
done   

