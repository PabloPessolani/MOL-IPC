#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_mol_*.txt
for i in {1..10}
do
    echo "loop_mol_ipc1 $i"
    /home/jara/mol-ipc/loops/affinity/loop_mol_ipc1 $1 | grep Throuhput  >> loop_mol_ipc1.txt
sleep 1
done

for i in {1..10}
do
    echo "loop_mol_ipc2 $i"
    /home/jara/mol-ipc/loops/affinity/loop_mol_ipc2 $1 | grep Throuhput >> loop_mol_ipc2.txt
sleep 1
done
for i in {1..10}
do
    echo "loop_mol_ipc3 $i"
    /home/jara/mol-ipc/loops/affinity/loop_mol_ipc3 $1 | grep Throuhput >> loop_mol_ipc3.txt
sleep 1
done

