#!/bin/sh

echo "Started $0"
TICK=0
while [ 1 ]; do
	echo "Tick $TICK!"
	TICK=$(($TICK+1))
	sleep 1
done
