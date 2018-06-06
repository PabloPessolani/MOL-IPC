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
./systask -d $dcid  > systask$lcl.txt 2> st_err$lcl.txt &
if [ $lcl -eq 0 ]
then 
################## PM NODE0 #####################
read  -p "PM Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/servers/pm
./pm $dcid > pm$lcl.txt 2> pm_err$lcl.txt &
sleep 2
################## RS NODE0 #####################
read  -p "RS Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/servers/rs
./rs $dcid > rs$lcl.txt 2> rs_err$lcl.txt &
sleep 2
####################### START WEBSRV NODE0 #################
#read  -p "WEBSRV Enter para continuar... "
#dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
#cd /home/MoL_Module/mol-ipc/commands/demonize
#./demonize -l node$lcl $lcl  $dcid 20 0 "/home/MoL_Module/mol-ipc/commands/m3urlget/websrv /home/MoL_Module/mol-ipc/commands/m3urlget/websrv.cfg" > websrv$lcl.txt 2> websrv$lcl.txt &
#sleep 2
#cat /proc/dvs/DC$dcid/procs
####################### START WEBCLT NODE0 #################
#read  -p "WEBCLT Enter para continuar... "
#dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
#cd /home/MoL_Module/mol-ipc/commands/demonize
#./demonize -l node$lcl $lcl  $dcid 21 0 "/home/MoL_Module/mol-ipc/commands/m3urlget/webclt fake.jpg"  > webclt$lcl.txt 2> webclt$lcl.txt
#sleep 2
#cat /proc/dvs/DC$dcid/procs
####################### START TTY NODE0 #################
#read  -p "TTY Enter para continuar... "
#dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
#cd /home/MoL_Module/mol-ipc/commands/demonize
#./demonize -l node$lcl $lcl  $dcid 5 0 "/home/MoL_Module/mol-ipc/tasks/tty/tty /home/MoL_Module/mol-ipc/tasks/tty/tty.cfg" > tty$lcl.txt 2> tty_err$lcl.txt &
#sleep 2
#cat /proc/dvs/DC$dcid/procs
####################### TEST_TTY NODE0 #################
#read  -p "TEST_TTY Enter para continuar... "
#dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
#cd /home/MoL_Module/mol-ipc/commands/demonize
#./demonize -l node$lcl $lcl  $dcid 21 0 "/home/MoL_Module/mol-ipc/tasks/tty/test_tty"  > test_tty$lcl.txt 2> test_tty_err$lcl.txt 
#dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
#sleep 2
#cat /proc/dvs/DC$dcid/procs
####################### START RDISK NODE0 #################
#read  -p "RDISK Enter para continuar... "
#dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
#cd /home/MoL_Module/mol-ipc/commands/demonize
#./demonize -l node$lcl $lcl  $dcid 3 0 "/home/MoL_Module/mol-ipc/tasks/rdisk/rdisk /home/MoL_Module/mol-ipc/tasks/rdisk/rdisk1.cfg" > rdisk$lcl.txt 2> rdisk_err$lcl.txt &
#sleep 2
#cat /proc/dvs/DC$dcid/procs
####################### CONFIG TAP & BR #################
read  -p "Config TAP and BR: Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
ifconfig | grep tap
mknod /dev/tap$dcid c 36 $[ 0 + 16 ]
chmod 666 /dev/tap$dcid
ls -l /dev/tap$dcid
# add bridge interface on host
brctl addbr br$dcid
# set bridge interface IP address 
ifconfig br$dcid 172.16.1.3 netmask 255.255.255.0 
read  -p "Configuring tap$dcid. Enter para continuar... "
# add new interface tap$dcid
ip tuntap add dev tap$dcid mode tap
# enable tap$dcid
ip link set dev tap$dcid up 
ifconfig | grep tap
# link tap interface to bridge
brctl addif br$dcid tap$dcid
sleep 2
cat /proc/dvs/DC$dcid/procs
####################### ETH #################
read  -p "ETHERNET Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
#demonize -{l|r|b} <vmid> <endpoint> <pid> <filename> <args...> 
./demonize -l node$lcl $lcl $dcid 6 0 "/home/MoL_Module/mol-ipc/tasks/eth/eth $dcid" > eth$lcl.txt 2> eth2$lcl.txt&
#
####################### CONFIG TAP9  #################
read  -p "Configuring tap9. Enter para continuar... "
mknod /dev/tap9 c 36 25
chmod 666 /dev/tap9
ls -l /dev/tap9
ip tuntap add dev tap9 mode tap
ip link set dev tap9 address 72:89:78:FF:88:EF
ip link set dev tap9 up 
ifconfig tap9 172.16.1.9 netmask 255.255.255.0 
ifconfig | grep tap9
brctl addif br0 tap9
#
####################### INET  #################
read  -p "INET Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
#demonize -{l|r|b} <vmid> <endpoint> <pid> "<filename> <args...>"
ifconfig > /home/MoL_Module/mol-ipc/ifconfig.txt
tcpdump -i tap$dcid -N -n -e -vvv -XX > /home/MoL_Module/mol-ipc/tcpdump.txt &
./demonize -l node$lcl $lcl $dcid 9 0 "/home/MoL_Module/mol-ipc/servers/inet/inet 0" > inet$lcl.txt &
sleep 2
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC$dcid/procs
####################### START FS NODE0 #################
read  -p "FS Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l node$lcl $lcl  $dcid 1 0 "/home/MoL_Module/mol-ipc/servers/fs/fs /home/MoL_Module/mol-ipc/servers/fs/molfs_DC$dcid.cfg" > fs$lcl.txt 2> fserr$lcl.txt &
sleep 2
cat /proc/dvs/DC$dcid/procs
####################### TEST_FS_INET  #################
read  -p "TEST_FS_INET Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
#demonize -{l|r|b} <vmid> <endpoint> <pid> <filename> <args...> 
read  -p "test_fs_inet_01: Enter para continuar... "
./demonize -l node$lcl $lcl  $dcid 20 0 "/home/MoL_Module/mol-ipc/molTestsLib/test_fs_inet_01" >> test_fs_inet_01.txt 2>> test_fs_inet_01.txt
sleep 5
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cat /proc/dvs/DC$dcid/procs
####################### TEST_INET  #################
read  -p "TEST_INET Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
#demonize -{l|r|b} <vmid> <endpoint> <pid> <filename> <args...> 
./demonize -l node$lcl $lcl $dcid 10 0 "/home/MoL_Module/mol-ipc/servers/inet/test_inet 0" > test_inet$lcl.txt &
sleep 2
cat /proc/dvs/DC$dcid/procs
ping -I tap9 -c 3 172.16.1.4
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
exit 
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#
####################### LOCAL INIT  #################
read  -p "LOCAL INIT Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
#demonize -{l|r|b} <vmid> <endpoint> <pid> <filename> <args...> 
./demonize -l node$lcl $lcl $dcid 11 1 "/home/MoL_Module/mol-ipc/servers/init/init 0" > init$lcl.txt &
sleep 2
cat /proc/dvs/DC$dcid/procs
####################### REMOTE INIT  #################
read  -p "REMOTE INIT Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
#demonize -R <vmid> <endpoint> <rmtnodeid> <filename> <args...>
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -R node$lcl $rmt $dcid 12 $rmt "/home/MoL_Module/mol-ipc/servers/init/init 0" > init$rmt.txt 2> error$rmt.txt &
sleep 2
cat /proc/dvs/DC$dcid/procs
exit
fi
################## NODE 1 #####################
sleep 2
cat /proc/dvs/DC$dcid/procs
exit

