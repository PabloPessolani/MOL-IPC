#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi
rm loop_unix_*.txt

for j in 36 512 1024 2048 4096 8192 
do
 for i in {1..10}
 do
    echo "[$i]loop_unix $j $1 "
	/home/jara/ipc-bench/unix_thr $j $1 | grep "msg/s" >> loop_unix_$j.txt
 sleep 1
 done
done
