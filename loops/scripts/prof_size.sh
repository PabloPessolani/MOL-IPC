#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm prof_mol_*.txt
dmesg -c > /dev/null
for j in 512 1024 2048 4096 8192 16384 32768 65536 131072
do
    echo "loop_mol_copy1 $i $j "
    /home/jara/mol-ipc/loops/loop_mol_copy1 $1 $j
	dmesg -c | grep PROF >> prof_size_$j.txt
done
