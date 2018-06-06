#!/bin/bash
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
mount | grep debug
read  -p "Spread Enter para continuar... "
/usr/local/sbin/spread  > /lib/init/rw/spread.txt &			
cd /home/MoL_Module/mol-module
insmod ./mol_replace.ko
lsmod | grep mol
cd /home/MoL_Module/mol-ipc
read  -p "DRVS Enter para continuar... "
/home/MoL_Module/mol-ipc/tests/test_dvs_end
/home/MoL_Module/mol-ipc/tests/test_dvs_init -n $lcl -V 2 -N 2 -P 96 -T 32 -S 64 -D 0
# 16777215
read  -p "DC$dcid Enter para continuar... "
/home/MoL_Module/mol-ipc/tests/test_dc_init -d $dcid -p 96 -t 32 -s 64 -n 2 -P 0 -m 1 
read  -p "TCP  PROXY Enter para continuar... "
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
echo 1 > /proc/sys/net/ipv4/tcp_low_latency
/home/MoL_Module/mol-ipc/proxy/tcp_proxy node$rmt $rmt > /lib/init/rw/node$rmt.txt 2> /lib/init/rw/error$rmt.txt &
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
dmesg -c  > /lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/tasks/systask/systask -d $dcid  > /lib/init/rw/systask$lcl.txt 2> /lib/init/rw/st_err$lcl.txt &
####################### COPY DISK IMAGE TO RAMDISK  #################
# 
read  -p "COPY IMAGE FILE Enter para continuar... "
cp /home/MoL_Module/mol-ipc/servers/diskImgs/FAT100M.img /lib/init/rw
################# START  RDISK  on NODE1 #################
read  -p "RDISK Enter para continuar... "
cp /home/MoL_Module/mol-ipc/tasks/rdisk/FAT100M.cfg /lib/init/rw /home/MoL_Module/mol-ipc/tasks/rdisk/rdisk /lib/init/rw/FAT100M.cfg 0 > /lib/init/rw/rdisk$lcl.txt 2> /lib/init/rw/rdisk_err$lcl.txt &
sleep 2
dmesg -c  >> /lib/init/rw/dmesg$lcl.txt
cat /proc/dvs/DC$dcid/procs
exit


