#!/bin/sh

CONF="runme.conf"
LOG="log.log"

rm counter-output
rm ticker-output
rm ticker-input
# Create a config
echo "ticker input file" > ticker-input
echo "$(pwd)/proc1.sh /dev/null /dev/null\n"\
"$(pwd)/counter.sh /dev/null $(pwd)/counter-output\n"\
"$(pwd)/ticker.sh tick_message $(pwd)/ticker-input $(pwd)/ticker-output" > "$CONF"

./myinit -c "$CONF" -l "$LOG"

MYINIT_PID=$(ps ax | grep "myinit -c" | grep -v "grep" | awk '{print $1}' )
echo "myinit PID: $MYINIT_PID"

CPNUM=$(($(ps --ppid $MYINIT_PID | wc -l) - 1)) # 1 line header, others children
echo "Child processes: $CPNUM"
echo "Waiting 5 seconds..."
sleep 5
CHILD2=$(ps --ppid $MYINIT_PID | sed "1,2d;4d" | awk '{print $1}')
echo "Killing the second child (PID $CHILD2)"
kill -s 9 $CHILD2
echo "Killed!"
sleep 1
CPNUM=$(($(ps --ppid $MYINIT_PID | wc -l) - 1)) # 1 line header, others children
echo "After 1 sec. Child processes: $CPNUM"

echo "OLD CONFIG CONTENTS:"
echo "-FILE START-----------------------------------------------------------"
cat "$CONF"
echo "-FILE END-------------------------------------------------------------"
echo "$(pwd)/proc1.sh /dev/null /dev/null" > "$CONF"
echo "NEW CONFIG CONTENTS:"
echo "-FILE START-----------------------------------------------------------"
cat "$CONF"
echo "-FILE END-------------------------------------------------------------"
kill -hup $MYINIT_PID
sleep .5
CPNUM=$(($(ps --ppid $MYINIT_PID | wc -l) - 1)) # 1 line header, others children
echo "Replaced config. Child processes: $CPNUM"

kill -int $MYINIT_PID

sleep 1
echo "Killed myinit"
echo "LOG FILE CONTENTS:"
cat "$LOG"
rm "$LOG"
rm "$CONF"
