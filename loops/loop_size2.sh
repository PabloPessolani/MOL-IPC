#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_size2_*.txt

for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..10}
 do
    echo "loop_mol_copy2 $i $j "
    /home/MoL_Module/mol-module/mol-ipc/loops/loop_mol_copy2 $1 $j | grep Throuhput >> loop_size2_$j.txt
 sleep 1
 done
done
