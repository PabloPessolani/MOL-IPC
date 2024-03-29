#!/bin/bash
if [ $# -ne 3 ]
then 
	echo "usage: $0  <nr_children> <loops> <nr_dcs>"
	echo $#
	exit 1
fi
# --------------------------------- test multi DC sendrec --------------------
dmesg -c > /dev/null
for (( dc=0; dc < $3; dc++ ))
	do
	echo "loop$_r-s_server: nr_children=$1 loops=$2 nr_dcs=$3" >  loop_msr_r-s-$dc.txt
done
#	
for i in {1..10}
	do
	for (( dc=0; dc < $3; dc++ ))
		do
		echo "[$i] loop_r-s_server $1 $2 $dc"
		/home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_r-s_server $1 $2 $dc >> loop_msr_r-s-$dc.txt 2> loop_err.txt &
#		let pid$dc=$!
#		echo "piddc=$pid$dc"
	done
	wait
	echo "all process exit"
#	for (( dc=0; dc < $3; dc++ ))
#		do
#		wait $pid$dc
#	done
	dmesg | grep ERROR
done
cat /proc/dvs/nodes >> loop_msr_r-s.txt
