#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi
rm loop_fifo_*.txt
for i in {1..10}
do
    echo "loop_fifo_ipc1 $i"
    /home/jara/tlpi-dist/pipes/loop_fifo_copy1 $1 36 | grep "loopbysec" >> loop_fifo_ipc1.txt
done
for i in {1..10}
do
    echo "loop_fifo_ipc3 $i"
    /home/jara/tlpi-dist/pipes/loop_fifo_copy1 $1 1  | grep "loopbysec" >> loop_fifo_ipc3.txt
done

for j in  36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..10}
 do
    echo "[$i]loop_fifo_copy1 $1 $j "
	/home/jara/tlpi-dist/pipes/loop_fifo_copy1 $1 $j | grep "loopbysec" >> loop_fifo_$j.txt
 done
done
