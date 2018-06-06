#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi
rm loop_lpc_*.txt
ifconfig lo 127.0.0.1
/home/jara/mol-ipc/rpc/mol-rpc_server &
read -p "STARTING THE RPC SERVER ON localhost"
for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..10}
 do
    echo "[$i]loop_rpc $j $1 "
	/home/jara/mol-ipc/rpc/mol-rpc_client localhost $j $1 | grep "Throuhput" >> loop_lpc_$j.txt
 done
done

