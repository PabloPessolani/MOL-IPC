#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_z*.txt
for k in 5 10 15 20
do
	for j in 36 512 1024 2048 4096 8192 16384 32768 65536
	do
 		for i in {1..5}
 		do
			let loops=$(( $1/$k))
    		echo "loop_zcopy_mperf $k $loops $j $i"
    		./loop_zcopy_mperf $k $loops $j | grep Throuhput >> loop_z$k-$j.txt
			cat /proc/dvs/VM0/procs
	 		sleep 1
 		done
	done
	read -p "Copiar achivos y luego continuar "
done



