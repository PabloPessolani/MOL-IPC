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
    /home/jara/mol-ipc/loops/loop_mol_ipc1 $1 | grep Throuhput  >> loop_mol_ipc1.txt
	cat /proc/dvs/VM0/procs
	sleep 2
done
read -p "Copiar achivos y luego continuar "

for i in {1..10}
do
    echo "loop_mol_ipc2 $i"
    /home/jara/mol-ipc/loops/loop_mol_ipc2 $1 | grep Throuhput >> loop_mol_ipc2.txt
	cat /proc/dvs/VM0/procs
	sleep 1
done
read -p "Copiar achivos y luego continuar "

for i in {1..10}
do
    echo "loop_mol_ipc3 $i"
    /home/jara/mol-ipc/loops/loop_mol_ipc3 $1 | grep Throuhput >> loop_mol_ipc3.txt
	cat /proc/dvs/VM0/procs
	sleep 1
done
read -p "Copiar achivos y luego continuar "

for i in {1..10}
do
    echo "loop_mol_zcopy1 $i"
    /home/jara/mol-ipc/loops/loop_mol_zcopy1 $1 4096 | grep Throuhput >> loop_mol_zcopy1.txt
	cat /proc/dvs/VM0/procs
	sleep 2
done
read -p "Copiar achivos y luego continuar "

for i in {1..10}
do
   	echo "loop_mol_zcopy2 $i"
	cat /proc/dvs/VM0/procs
    	/home/jara/mol-ipc/loops/loop_mol_zcopy2 $1 4096 | grep Throuhput >> loop_mol_zcopy2.txt
	sleep 1
done





