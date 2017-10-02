#!/bin/bash


curl -X POST -d "id=$1&amount=1.7" http://127.0.0.1:6502/api/user/deal
