#!/bin/sh

LOCKSTAT="lockstat.txt"

rm -f *.lck

echo "Testing lock file removal..."

./task2 &
T2PID="$!"
echo "PID: $T2PID"
sleep .6
echo "runme.sh: Removing lock file!"
rm test.txt.lck
sleep 2
echo "Not killing the process: it crases as supposed to"
if ps -p $T2PID
then
	echo "Process didn't crash!"
	kill -int $T2PID
	exit 1
fi

echo "Waiting 5 seconds before continuing to the next test..."
sleep 5

rm "$LOCKSTAT"
touch "$LOCKSTAT"

./task2 -n 1 -S "$LOCKSTAT" &
PIDS_1="$!"
echo "Started process #1 with PID $PIDS_1"
./task2 -n 2 -S "$LOCKSTAT" &
PIDS_2="$!"
echo "Started process #2 with PID $PIDS_2"
./task2 -n 3 -S "$LOCKSTAT" &
PIDS_3="$!"
echo "Started process #3 with PID $PIDS_3"
./task2 -n 4 -S "$LOCKSTAT" &
PIDS_4="$!"
echo "Started process #4 with PID $PIDS_4"
./task2 -n 5 -S "$LOCKSTAT" &
PIDS_5="$!"
echo "Started process #5 with PID $PIDS_5"
./task2 -n 6 -S "$LOCKSTAT" &
PIDS_6="$!"
echo "Started process #6 with PID $PIDS_6"
./task2 -n 7 -S "$LOCKSTAT" &
PIDS_7="$!"
echo "Started process #7 with PID $PIDS_7"
./task2 -n 8 -S "$LOCKSTAT" &
PIDS_8="$!"
echo "Started process #8 with PID $PIDS_8"
./task2 -n 9 -S "$LOCKSTAT" &
PIDS_9="$!"
echo "Started process #9 with PID $PIDS_9"
./task2 -n 10 -S "$LOCKSTAT" &
PIDS_10="$!"
echo "Started process #10 with PID $PIDS_10"

sleep 120 # 120 seconds = 5 minutes

kill -int $PIDS_1
echo "Killing #1 with PID $PIDS_1"
kill -int $PIDS_2
echo "Killing #2 with PID $PIDS_2"
kill -int $PIDS_3
echo "Killing #3 with PID $PIDS_3"
kill -int $PIDS_4
echo "Killing #4 with PID $PIDS_4"
kill -int $PIDS_5
echo "Killing #5 with PID $PIDS_5"
kill -int $PIDS_6
echo "Killing #6 with PID $PIDS_6"
kill -int $PIDS_7
echo "Killing #7 with PID $PIDS_7"
kill -int $PIDS_8
echo "Killing #8 with PID $PIDS_8"
kill -int $PIDS_9
echo "Killing #9 with PID $PIDS_9"
kill -int $PIDS_10
echo "Killing #10 with PID $PIDS_10"

sleep 3
echo "================================================================="
echo "LOCK STATISTICS"
cat "$LOCKSTAT"

rm "$LOCKSTAT"
rm "test.txt"
