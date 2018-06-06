#!/bin/bash
if [ $# -ne 2 ]
then 
	echo "usage: $0 <nr_forks> <loops>"
	echo $#
	exit 1
fi
rm loop_*.txt
# --------------------------------- test sendrec --------------------
echo "loop_r-s_server:  test sendrec/receive-send  nr_forks=$1 loops=$2"  >> loop_sr_r-s.txt
for i in {1..10}
 do
    echo "[$i] loop_r-s_server $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_r-s_server $1 $2 | grep Throuhput >> loop_sr_r-s.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
#read -p "Enter para continuar... "
# --------------------------------- test rcvrqst/reply  --------------------
echo "loop_r-r_server: test rcvrqst/reply  nr_forks=$1 loops=$2" >> loop_sr_r-r.txt
for i in {1..10}
 do
    echo "[$i] loop_r-r_server $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_r-r_server $1 $2 | grep Throuhput >> loop_sr_r-r.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
#read -p "Enter para continuar... "
# ---------------------------- test send/receive ------------------ 
echo "loop_r-s_server:  test send-receive/receive-send  nr_forks=$1 loops=$2" >> loop_s-r_r-s.txt
for i in {1..10}
 do
    echo "[$i] loop_r-s_server $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_r-s_server $1 $2  | grep Throuhput >> loop_s-r_r-s.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
#read -p "Enter para continuar... "
# ------------------------------ test notify/receive --------------
echo "loop_r-n_server:  test notify/receive  nr_forks=$1 loops=$2" >> loop_n-r_r-n.txt
for i in {1..10}
 do
    echo "[$i] loop_r-n_server $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_r-n_server $1 $2 | grep Throuhput >> loop_n-r_r-n.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
read -p "Enter para continuar... "
cat /proc/dvs/node1/stats >> loop_vcopy_stats.txt
