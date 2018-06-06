#!/bin/bash
rm loop_clt_vcopy_*.txt
read -p "Enter para continuar... ESPERAR QUE AVISE NODO 0 "
for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..5}
 do
    echo "[$i]loop_clt_vcopy1 2 $j "
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_clt_vcopy1 2 $j | grep Throuhput >> loop_clt_vcopy1_$j.txt
	cat /proc/dvs/DC0/procs
	sleep 1
 done
done
dmesg | grep ERROR
#read -p "Enter para continuar... "
