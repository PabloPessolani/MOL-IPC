#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <lcl_nodeid>"
	exit 1 
fi
lcl=$1
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
read  -p "TCP PROXY Enter para continuar... "
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
./systask -d $dcid  > st_out$lcl.txt 2> st_err$lcl.txt &
####################### CONFIG TAP & BR #################
echo 1 >  /proc/sys/net/ipv4/ip_forward
read  -p "Config TAP and BR: Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
ifconfig | grep tap
mknod /dev/tap$dcid c 36 $[ 0 + 16 ]
mknod /dev/tap1  c 36 $[ 1 + 16 ]
chmod 666 /dev/tap$dcid
chmod 666 /dev/tap1
ls -l /dev/tap*
# add bridge interface on host
brctl addbr br$dcid
# set bridge interface IP address 
ifconfig br$dcid 172.16.$lcl.3 netmask 255.255.255.0 
read  -p "Configuring tap$dcid. Enter para continuar... "
# add new interface tap$dcid
ip tuntap add dev tap$dcid mode tap
ip tuntap add dev tap1 mode tap
# enable tap$dcid
ip link set dev tap$dcid up 
ip link set dev tap1 up 
# link tap interface to bridge
brctl addif br0 tap$dcid
brctl addif br0 tap1
ifconfig tap
####################### M3ETH #################
read  -p "M3ETH Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/tasks/m3eth
./m3eth $dcid > m3eth_out$lcl.txt 2> m3eth_err$lcl.txt&
sleep 2
cat /proc/dvs/DC$dcid/procs
####################### START LWIP #################
tcpdump -i tap$dcid -N -n -e -vvv -XX > /home/MoL_Module/mol-ipc/tcpdump.txt &
read  -p "Starting lwip-tap. Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/lwip-mol
./lwip-mol -H -d -i vmid=$dcid,name=mol0,addr=172.16.$lcl.2,netmask=255.255.255.0,gw=172.16.$lcl.1  > lwipout.txt 2> lwiperr.txt &
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC$dcid/procs
ifconfig tap$dcid
ping -c 5 172.16.$lcl.2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
exit
