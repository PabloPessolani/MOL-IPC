#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm prof_mol_*.txt
dmesg -c > /dev/null
/home/jara/mol-ipc/loops/loop_mol_ipc1 $1 
dmesg -c | grep PROF >> prof_mol_ipc1.txt
/home/jara/mol-ipc/loops/loop_mol_ipc2 $1 
dmesg -c | grep PROF >> prof_mol_ipc2.txt
/home/jara/mol-ipc/loops/loop_mol_ipc2 $1 
dmesg -c | grep PROF>> prof_mol_ipc3.txt

