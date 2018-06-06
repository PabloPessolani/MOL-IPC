#!/bin/bash
lcl=0
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
mkdir /lib/init/rw/nweb
cp /home/nweb/*  /lib/init/rw/nweb
ls -l /lib/init/rw/nweb
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
echo "START m3ftp1.sh on NODE1"
read  -p "Enter para continuar... "
####################### START M3FTPD #################
read  -p "Starting M3FTPD. Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l $lcl $dcid 20 0 "/home/MoL_Module/mol-ipc/servers/m3ftp/m3ftpd /lib/init/rw/nweb"  > ftpsrvout.txt 2> ftpsrverr.txt &
sleep 2
cat /proc/dvs/DC$dcid/procs
####################### START M3FTP on NODE1 #################
read  -p "Starting M3FTPD on NODE1. Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l $rmt $dcid 21 0 "/home/MoL_Module/mol-ipc/servers/m3ftp/m3ftp -g 20 /lib/init/rw/nweb/file10M.txt cfile10M.txt"  > ftpcltout.txt 2> ftpclterr.txt &
sleep 10
####################### START M3FTP on NODE1 #################
read  -p "Starting M3FTPD on NODE1. Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
./demonize -l $rmt $dcid 22 0 "/home/MoL_Module/mol-ipc/servers/m3ftp/m3ftp -g 20 /lib/init/rw/nweb/file50M.txt cfile50M.txt"  > ftpcltout.txt 2> ftpclterr.txt &
sleep 2
echo "this program get /lib/init/rw/nweb/file50M.txt from ftpd"
echo " the result is wrote in servers servers/m3ftp/cfile50M.txt on NODE1"
echo "performance results in NODE0  servers/m3ftp/m3ftpd.txt"
cat /proc/dvs/DC$dcid/procs
exit

