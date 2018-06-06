#!/bin/bash
if [ $# -ne 2 ]
then 
	echo "usage: $0 <nr_forks> <loops>"
	echo $#
	exit 1
fi
rm loop_copy*.txt
#--------------------------- NODE0(Rqtr=src) NODO1(dst) ---------------------------------------
for j in 512 1024 2048 4096 8192 16384 32768 65536 131072
do
 for i in {1..5}
 do
    echo "[$i]loop_copy_server $1 $2 $j "
  /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_copy_server $1 $2 $j | grep Throuhput >> loop_copy$1_$j.txt
	cat /proc/dvs/DC0/procs
 done
done
dmesg | grep ERROR
#read -p "Enter para continuar... "

