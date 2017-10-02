#!/bin/bash
COUNTER=0
while [ $COUNTER -lt 10000 ]; do
i=$COUNTER
echo $i
curl -X POST -d "id=$i&name=$i" http://127.0.0.1:6502/api/user/registered
curl -X POST -d "id=$i&name=$i" http://127.0.0.1:6502/api/user/connected
let COUNTER=COUNTER+1
done   

