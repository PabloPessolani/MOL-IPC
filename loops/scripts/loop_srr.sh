#/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <loops>"
	echo "number of args = $#"
	exit 1
fi
rm loop_srr_*.txt
/home/jara/simpl/benchmarks/bin/receiver -n RECEIVER &
for j in 36 512 1024 2048 4096 8192
do
 for i in {1..10}
 do
    echo "loop_srr $i $j "
    /home/jara/simpl/benchmarks/bin/sender -n SENDER -r RECEIVER -t $1 -s  $j | grep Throuhput >> loop_srr_$j.txt
 sleep 1
 done
done

