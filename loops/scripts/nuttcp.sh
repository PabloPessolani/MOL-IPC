#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi
rm nuttcp*.txt

for j in 36 512 1024 2048 4096 8192 16384 32768 65536
do
 for i in {1..10}
 do
    echo "[$i]nuttcp $j $1 "
	nuttcp -t -l $j -n $1 node1 | grep "Mbps" > nuttcp_$j.txt
 done
done
