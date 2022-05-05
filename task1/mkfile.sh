#!/bin/sh

SIZE=$((4 * 1024 * 1024 + 1))
echo "Creating a file, size: $SIZE bytes"
fallocate -l $SIZE "$1" > /dev/null
printf "\x1" | dd of="$1" bs=1 seek=0 conv=notrunc > /dev/null 2>&1
printf "\x1" | dd of="$1" bs=1 seek=10000 conv=notrunc > /dev/null 2>&1
printf "\x1" | dd of="$1" bs=1 seek=$(($SIZE-1)) conv=notrunc > /dev/null 2>&1
