#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo $#
	exit 1
fi
rm loop_simpl_*.txt
/home/jara/simpl/benchmarks/bin/receiver -n RECEIVER &
for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..10}
 do
    echo "[$i]loop_simpl $j $1 "
    /home/jara/simpl/benchmarks/bin/sender -n SENDER -r RECEIVER -t $1 -s $j | grep Throuhput >> loop_simpl_$j.txt
 done
done
