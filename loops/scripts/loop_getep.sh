#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_mol_getep*.txt
for i in {1..10}
do
    echo "loop_mol_getep $i"
    /home/jara/mol-ipc/loops/loop_mol_getep $1 | grep Throuhput  >> loop_mol_getep.txt
	sleep 1
done
