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
    echo "loop_mol_ipc1a $i"
    ./loop_mol_ipc1a $1 | grep Throuhput  >> loop_mol_ipc1a.txt
sleep 2
done

for i in {1..10}
do
    echo "loop_mol_ipc2a $i"
    ./loop_mol_ipc2a $1 | grep Throuhput >> loop_mol_ipc2a.txt
sleep 2
done
