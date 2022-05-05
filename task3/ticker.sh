#!/bin/sh

read -r MES
echo "Message read from file: $MES"

while [ 1 ]; do
	echo "Tick message $1!"
	sleep 1
done
