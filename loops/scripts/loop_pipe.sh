#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi
rm loop_pipe_*.txt

for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..10}
 do
    echo "[$i]loop_pipe $j $1 "
	/home/jara/ipc-bench/pipe_thr $j $1 | grep "msg/s" >> loop_pipe_$j.txt
	sleep 1
 done
done
