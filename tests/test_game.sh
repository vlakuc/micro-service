#!/bin/bash
NUM_USERS=15
COUNTER=0
while [ $COUNTER -lt $NUM_USERS ]; do
i=$COUNTER
echo $i
curl -X POST -d "id=$i&name=$i" http://127.0.0.1:6502/api/user/registered
curl -X POST -d "id=$i&name=$i" http://127.0.0.1:6502/api/user/connected
let COUNTER=COUNTER+1
done   

while true
do
 id=`shuf -i1-$NUM_USERS -n1`
 curl -X POST -d "id=${id}&amount=0.001" http://127.0.0.1:6502/api/user/deal
done
