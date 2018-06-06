#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <proxy_type>"
	echo "u: udp"
	echo "t: tcp"
	echo "T: TIPC"
	exit 1 
fi
dmesg -c
if [ $1 == "u" ]
then 
	echo "UDP proxies selected"
elif  [ $1 == "t" ]
then
	echo "TCP proxies selected"
elif  [ $1 == "T" ]
then
	echo "TIPC proxies selected"
else
	echo "usage: $0 <proxy_type>"
	echo "u: udp"
	echo "t: tcp"
	echo "T: TIPC"
	exit 1
fi
/home/jara/mol-ipc/tests/test_drvs_end
/home/jara/mol-ipc/tests/test_drvs_init  -n 1 -D 0
# 16777215 
/home/jara/mol-ipc/tests/test_vm_init -v 0
read  -p "Enter para continuar... "
if [ $1 == "u" ]
then 
	/home/jara/mol-ipc/proxy/uproxy node0 0  >node0.txt 2>error0.txt &
	/home/jara/mol-ipc/proxy/uproxy node2 2  >node2.txt 2>error2.txt &
elif  [ $1 == "t" ]
then 
	/home/jara/mol-ipc/proxy/tproxy node0 0  >node0.txt 2>error0.txt &
	/home/jara/mol-ipc/proxy/tproxy node2 2  >node2.txt 2>error2.txt &
else
	tipc-config -net=4711 -a=1.1.101 -be=eth:eth0
	/home/jara/mol-ipc/proxy/tipcproxy node0 0  >node0.txt 2>error0.txt &
#	/home/jara/mol-ipc/proxy/tipcproxy node2 2  >node2.txt 2>error2.txt &
fi
sleep 10
read  -p "Enter para continuar... "
cat /proc/drvs/nodes
cat /proc/drvs/proxies/info
cat /proc/drvs/proxies/procs
read -p "Enter para continuar... "
/home/jara/mol-ipc/tests/test_add_node 0 0
#/home/jara/mol-ipc/tests/test_add_node 0 2
cat /proc/drvs/VM0/info
read -p "Enter para continuar... "
/home/jara/mol-ipc/tests/test_rmtbind 0 0 0
#/home/jara/mol-ipc/tests/test_rmtbind 0 2 2
#/home/jara/mol-ipc/tests/test_rmtbind 0 10 0
cat /proc/drvs/VM0/procs







