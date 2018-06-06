#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_sr_mpeer*.txt
for k in 4 8 12 16 
do
	for i in {1..5}
	do
	    let loops=$1
    	echo "[$i]loop_sr_mpeer $k $loops"
    	/home/MoL_Module/mol-module/mol-ipc/loops/loop_sr_mpeer $k $loops | grep Throuhput  >> loop_sr_mpeer$k.txt
		cat /proc/dvs/DC0/procs
		sleep 1
	done
done
rm loop_rr_mpeer*.txt
for k in 4 8 12 16 
do
	for i in {1..5}
	do
	    let loops=$1
    	echo "[$i]loop_rr_mpeer $k $loops"
    	/home/MoL_Module/mol-module/mol-ipc/loops/loop_rr_mpeer $k $loops | grep Throuhput  >> loop_rr_mpeer$k.txt
		cat /proc/dvs/DC0/procs
		sleep 1
	done
done
exit






