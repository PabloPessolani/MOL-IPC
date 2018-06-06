#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_mol_bind*.txt
for i in {1..10}
do
    echo "loop_mol_bind $i"
    /home/jara/mol-ipc/loops/loop_mol_bind $1 | grep Throuhput  >> loop_mol_bind.txt
	sleep 1
done
