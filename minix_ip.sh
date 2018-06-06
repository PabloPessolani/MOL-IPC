#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <lcl_nodeid>"
	exit 1 
fi
vmid=0
lcl=$1
let rmt=(1 - $lcl)
echo "lcl=$lcl rmt=$rmt" 
# enable routing between interfaces
echo 1 >  /proc/sys/net/ipv4/ip_forward
# add bridge interface on host
brctl addbr br0
# set bridge interface IP address 
ifconfig br0 172.16.$1.1 netmask 255.255.255.0 
read  -p "Configuring tap0. Enter para continuar... "
# add new interface tap0
ip tuntap add dev tap0 mode tap
# enable tap0
ip link set dev tap0 up 
# link tap interface to bridge
brctl addif br0 tap0
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
################## SYSTASK NODE0 #####################
read  -p "SYSTASK Enter para continuar... "
dmesg -c  > /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/tasks/systask
./systask -d $dcid  > systask$lcl.txt 2> st_err$lcl.txt &
################# ETH  NODE0 #################
read  -p "ETH Enter para continuar... "
cd /home/MoL_Module/mol-ipc/tasks/eth
./eth  $dcid > eth$lcl.txt 2> eth_err$lcl.txt &
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC$dcid/procs
################## PM NODE0 #####################
read  -p "PM Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/servers/pm
./pm $dcid > pm$lcl.txt 2> pm_err$lcl.txt &
cat /proc/dvs/DC0/procs
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
################## RS NODE0 #####################
read  -p "RS Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/servers/rs
./rs $dcid > rs$lcl.txt 2> rs_err$lcl.txt &
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC0/procs
################## INET NODE0 #####################
read  -p "INET Enter para continuar... "
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l node$lcl $lcl $dcid 9 9 "/home/MoL_Module/mol-ipc/servers/inet/inet" > inet$lcl.txt 2> ineterr$lcl.txt &
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC0/procs
################## FS NODE0 #####################
read  -p "FS Enter para continuar... "
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l node$lcl $lcl $dcid 1 1 "/home/MoL_Module/mol-ipc/servers/fs/fs /home/MoL_Module/mol-ipc/servers/fs/molfs_DC0.cfg" > fs$lcl.txt 2> fserr$lcl.txt
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC0/procs
################## MNX_IFCONFIG NODE0 #####################
#read  -p "MNX_IFCONFIG Enter para continuar... "
#cd /home/MoL_Module/mol-ipc/commands/demonize
#./demonize -l node$lcl $lcl $dcid 15 0 "/home/MoL_Module/mol-ipc/commands/simple/mnx_ifconfig -I /dev/ip " > ifcout$lcl.txt 2> ifcerr$lcl.txt
#sleep 2
#dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
# cat /proc/dvs/DC0/procs
################## MNX_IFCONFIG NODE0 #####################
read  -p "MNX_IFCONFIG Enter para continuar... "
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l node$lcl $lcl $dcid 16 0 "/home/MoL_Module/mol-ipc/commands/simple/mnx_ifconfig -h 172.16.1.1 " > if2cout$lcl.txt 2> if2cerr$lcl.txt
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC0/procs
################## MNX_IFCONFIG NODE0 #####################
read  -p "MNX_IFCONFIG Enter para continuar... "
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l node$lcl $lcl $dcid 17 0 "/home/MoL_Module/mol-ipc/commands/simple/mnx_ifconfig" > ifcout$lcl.txt 2> ifcerr$lcl.txt
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC0/procs
