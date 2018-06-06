#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi
rm loop_rpc_*.txt

read -p "START THE RPC SERVER ON node1 "
for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..10}
 do
    echo "[$i]loop_rpc $j $1 "
	/home/jara/mol-ipc/rpc/mol-rpc_client node1 $j $1 | grep "Throuhput" >> loop_rpc_$j.txt
 done
done