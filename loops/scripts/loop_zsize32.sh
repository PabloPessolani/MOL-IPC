#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_size_*.txt

for i in {1..10}
do
    echo "loop_mol_zcopy1 $i $j "
    ./loop_mol_zcopy1 $1 32 >> loop_size_z32.txt
sleep 1
done
