#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <1|2|3|4|5|6>"
	echo " IT MUST RUN ON NODE1"	
	echo " 1: test #1 Webserver on NODE0, MOL-FS on NODE0 and its image file"
	echo " 2: test #2 Webserver on NODE0, MOL-FS on NODE0 and RDISK on NODE0"
	echo " 3: test #3 Webserver on NODE0, MOL-FS on NODE0 and RDISK on NODE1"
	echo " 4: test #4 Webserver on NODE0, MOL-FS on NODE1 and its image file"
	echo " 5: test #5 Webserver on NODE0, MOL-FS on NODE1 and RDISK on NODE1"
	echo " 6: test #6 Webserver on NODE0, MOL-FS on NODE1 and RDISK on NODE2"
	exit 1 
fi
if [ $1 -lt 0 ]
then
	echo "usage: $0 <1|2|3|4|5|6>"
	exit 1 
fi
if [ $1 -gt 6 ]
then
	echo "usage: $0 <1|2|3|4|5|6>"
	exit 1 
fi
echo "Ready for running test $1.."
echo " 1: test #1 Webserver on NODE0, MOL-FS on NODE0 and its image file"
echo " 2: test #2 Webserver on NODE0, MOL-FS on NODE0 and RDISK on NODE0"
echo " 3: test #3 Webserver on NODE0, MOL-FS on NODE0 and RDISK on NODE1"
echo " 4: test #4 Webserver on NODE0, MOL-FS on NODE1 and its image file"
echo " 5: test #5 Webserver on NODE0, MOL-FS on NODE1 and RDISK on NODE1"
echo " 6: test #6 Webserver on NODE0, MOL-FS on NODE1 and RDISK on NODE2"
read  -p "Enter para continuar... "
if [ $1 -lt 3 ]
then
	echo  "Nada para hacer ... "
	exit
fi
lcl=1
vmid=0
let rmt=(1 - $lcl)
echo "lcl=$lcl rmt=$rmt" 
read  -p "Enter para continuar... "
dmesg -c > /dev/null
X86DIR="/sys/kernel/debug/x86"
if [ ! -d $X86DIR ]; then
	echo mounting debugfs...
	mount -t debugfs none /sys/kernel/debug
else 
	echo debugfs is already mounted
fi
cp /home/minix180M.img  /lib/init/rw/
ls -l /lib/init/rw/
mount | grep debug
read  -p "Spread Enter para continuar... "
/usr/local/sbin/spread  > spread.txt &			
cd /home/MoL_Module/mol-module
insmod ./mol_replace.ko
lsmod | grep mol
cd /home/MoL_Module/mol-ipc
read  -p "DRVS Enter para continuar... "
/home/MoL_Module/mol-ipc/tests/test_dvs_end
/home/MoL_Module/mol-ipc/tests/test_dvs_init -n $lcl -D 16777215
read  -p "DC$dcid Enter para continuar... "
/home/MoL_Module/mol-ipc/tests/test_dc_init -d $dcid -P 0 -m 1
read  -p "TCP  PROXY Enter para continuar... "
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
echo 1 > /proc/sys/net/ipv4/tcp_low_latency
cd /home/MoL_Module/mol-ipc/proxy
./tcp_proxy node$rmt $rmt >node$rmt.txt 2>error$rmt.txt &
sleep 5
read  -p "Enter para continuar... "
cat /proc/dvs/nodes
cat /proc/dvs/proxies/info
cat /proc/dvs/proxies/procs
read  -p "ADDNODE Enter para continuar... "
cat /proc/dvs/DC$dcid/info
/home/MoL_Module/mol-ipc/tests/test_add_node $dcid $rmt
################## SYSTASK NODE1 #####################
read  -p "SYSTASK Enter para continuar... "
dmesg -c  > /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/tasks/systask
./systask -d $dcid  > systask$lcl.txt 2> st_err$lcl.txt &
################# START  RDISK  on NODE1 #################
if [ $1 -gt 2 ]
then
	read  -p "RDISK Enter para continuar... "
	cd /home/MoL_Module/mol-ipc/tasks/rdisk
	./rdisk rdisk.cfg 0 > rdisk$lcl.txt 2> rdisk_err$lcl.txt &
	sleep 2
	dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
	cat /proc/dvs/DC$dcid/procs
fi
route add -net 172.16.0.0 gw node0 netmask 255.255.255.0
cat /proc/dvs/DC$dcid/info
exit
