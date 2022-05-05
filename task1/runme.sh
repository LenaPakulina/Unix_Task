#!/bin/sh

stat_file() {
	BLOCKS=$(stat --printf="%b" "$1")
	BLOCK_SIZE=$(stat --printf="%B" "$1")
	L_SIZE=$(stat --printf="%s" "$1")
	echo "$1:"
	echo "  Logical size: $L_SIZE bytes"
	echo "  $BLOCKS x $BLOCK_SIZE byte blocks -> $(($BLOCKS * $BLOCK_SIZE)) bytes of disk space used"
}

echo "Creating fileA"
./mkfile.sh fileA

echo "Creating fileB"
./task1 fileA fileB

echo "Creating fileA.gz"
gzip fileA -c > fileA.gz
echo "Creating fileB.gz"
gzip fileB -c > fileB.gz

echo "Unpacking fileB.gz | Packing fileC"
gzip -dc fileB.gz | ./task1 fileC

echo "Creating fileD with block size 100"
./task1 -b 100 fileA fileD


echo "=============================================="
stat_file fileA
stat_file fileB
stat_file fileA.gz
stat_file fileB.gz
stat_file fileC
stat_file fileD

rm -f fileA
rm -f fileB
rm -f fileA.gz
rm -f fileB.gz
rm -f fileC
rm -f fileD
