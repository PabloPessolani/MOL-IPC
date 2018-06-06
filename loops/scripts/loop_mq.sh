#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi

rm loop_mq_*.txt

for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
	echo $j > /proc/sys/kernel/msgmnb

 for i in {1..10}
 do
    echo "[$i]loop_mq  $j $1 "
	/home/jara/mol-ipc/loops/loop_mq_copy1 $j $1 | grep Throuhput >> loop_mq_$j.txt
 done
done

