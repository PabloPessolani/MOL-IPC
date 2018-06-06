#!/bin/bash
if [ $# -ne 2 ]
then 
	echo "usage: $0 <nr_forks> <loops>"
	echo $#
	exit 1
fi
# --------------------------------- test sendrec --------------------
rm loop_sr_r-s.txt
echo "loop_sr_client:  test sendrec/receive-send  nr_forks=$1 loops=$2" 
for i in {1..10}
 do
    echo "[$i] loop_sr_client $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_sr_client $1 $2 | grep Throuhput >> loop_sr_r-s.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
#read -p "Enter para continuar... "
# --------------------------------- test rcvrqst/reply  --------------------
rm loop_sr_r-r.txt
echo "loop_sr_client: test sendrec/rcvrqst-reply nr_forks=$1 loops=$2" 
for i in {1..10}
 do
    echo "[$i] loop_sr_client $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_sr_client $1 $2 | grep Throuhput >> loop_sr_r-r.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
#read -p "Enter para continuar... "
# ---------------------------- test send/receive ------------------ 
rm loop_s-r_r-s.txt
echo "loop_s-r_client:  test send-receive/receive-send  nr_forks=$1 loops=$2" 
for i in {1..10} 
 do
    echo "[$i] loop_s-r_client $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_s-r_client $1 $2 | grep Throuhput >> loop_s-r_r-s.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
#read -p "Enter para continuar... "
# ------------------------------ test notify/receive --------------
rm loop_n-r_r-n.txt
echo "loop_n-r_client:  test notify/receive  nr_forks=$1 loops=$2" >> loop_n-r_r-n.txt
for i in {1..10}
 do
    echo "[$i] loop_n-r_client $1 $2"
    /home/MoL_Module/mol-module/mol-ipc/proxy/test/loop_n-r_client $1 $2 | grep Throuhput >> loop_n-r_r-n.txt
	cat /proc/dvs/DC0/procs
done
dmesg | grep ERROR
read -p "Enter para continuar... "
cat /proc/dvs/node1/stats >> loop_vcopy_stats.txt
