#!/bin/bash
if [ $# -ne 1 ]
then 
	echo "usage: $0 <1|2|3>"
	echo " 1: test #1 m3copy/copy on NODE0, FATFS on NODE0 and its image file"
	echo " 2: test #2 m3copy/copy on NODE0, FATFS on NODE0 and RDISK on NODE0"
	echo " 3: test #3 m3copy/copy on NODE0, FATFS on NODE0 and RDISK on NODE1"
	exit 1 
fi
if [ $1 -lt 0 ]
then
	echo "usage: $0 <1|2|3>"
	exit 1 
fi
if [ $1 -gt 3 ]
then
	echo "usage: $0 <1|2|3>"
	exit 1 
fi
echo "Ready for running test $1.."
echo " 1: test #1 m3copy on NODE0, FATFS on NODE0 and its image file"
echo " 2: test #2 m3copy on NODE0, FATFS on NODE0 and RDISK on NODE0"
echo " 3: test #3 m3copy on NODE0, FATFS on NODE0 and RDISK on NODE1"
read  -p "Enter para continuar... "
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
################## SYSTASK NODE0 #####################
read  -p "SYSTASK Enter para continuar... "
dmesg -c  > /lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/tasks/systask/systask -d $dcid  > /lib/init/rw/systask$lcl.txt 2> /lib/init/rw/st_err$lcl.txt &
####################### COPY DISK IMAGE TO RAMDISK  #################
# 
if [ $1 -lt 3 ]
then
	read  -p "COPY IMAGE FILE Enter para continuar... "
	cp /home/MoL_Module/mol-ipc/servers/diskImgs/FAT100M.img /lib/init/rw
fi
################# OPTION 2: START  RDISK  on NODE0 #################
if [ $1 -eq 2 ]
then
	read  -p "RDISK Enter para continuar... "
	cp /home/MoL_Module/mol-ipc/tasks/rdisk/FAT100M.cfg /lib/init/rw 
	/home/MoL_Module/mol-ipc/tasks/rdisk/rdisk /lib/init/rw/FAT100M.cfg 0 > /lib/init/rw/rdisk$lcl.txt 2> /lib/init/rw/rdisk_err$lcl.txt &
	sleep 2
	dmesg -c  >> /lib/init/rw/dmesg$lcl.txt
	cat /proc/dvs/DC$dcid/procs
fi 
if [ $1 -gt 2 ]
then
	read  -p "Start RDISK on NODE1 Enter para continuar... "
fi
################## PM NODE0 #####################
read  -p "PM Enter para continuar... "
dmesg -c  >> /lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/servers/pm/pm $dcid > /lib/init/rw/pm$lcl.txt 2> /lib/init/rw/pm_err$lcl.txt &
sleep 2
################## RS NODE0 #####################
read  -p "RS Enter para continuar... "
dmesg -c  >> /lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/servers/rs/rs $dcid > /lib/init/rw/rs$lcl.txt 2> /lib/init/rw/rs_err$lcl.txt &
sleep 2
####################### START FATFS  #################
#  START FATFS NODE0
read  -p "FATFS Enter para continuar... "
if [ $1 -eq 1 ]
then
	cp /home/MoL_Module/mol-ipc/servers/fatFS/FATFS_IMAGE.cfg  /lib/init/rw/FATFS_NODE0.cfg
else
	cp /home/MoL_Module/mol-ipc/servers/fatFS/FATFS_RDISK.cfg  /lib/init/rw/FATFS_NODE0.cfg
fi 	
dmesg -c  >> /lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/commands/demonize/demonize -l $lcl $lcl $dcid 1 0 "/home/MoL_Module/mol-ipc/servers/fatFS/fatFS /lib/init/rw/FATFS_NODE0.cfg" > /lib/init/rw/fatfs$lcl.txt 2> /lib/init/rw/fatfserr$lcl.txt &
sleep 2
cat /proc/dvs/DC$dcid/procs
####################### START M3COPY 5M #################
#read  -p "M3COPY 5M Enter para continuar... "
#dmesg -c  >>/lib/init/rw/dmesg$lcl.txt
#/home/MoL_Module/mol-ipc/commands/demonize/demonize -l $lcl $lcl $dcid 21 0 "/home/MoL_Module/mol-ipc/commands/m3copy/m3copy -g 5Mb.txt /dev.ro/shm/linux_5Mb.txt" > /lib/init/rw/m3copy$lcl.txt 2> /lib/init/rw/m3copy$lcl.txt &
#sleep 5
#cat /proc/dvs/DC$dcid/procs
# rm /dev.ro/shm/linux_5Mb.txt
####################### START M3COPY 10M #################
read  -p "M3COPY 10M Enter para continuar... "
dmesg -c  >>/lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/commands/demonize/demonize -l $lcl $lcl $dcid 21 0 "/home/MoL_Module/mol-ipc/commands/m3copy/m3copy -g 10Mb.txt /dev.ro/shm/linux_10Mb.txt" > /lib/init/rw/m3copy$lcl.txt 2> /lib/init/rw/m3copy$lcl.txt &
sleep 10
cat /proc/dvs/DC$dcid/procs
#rm /dev.ro/shm/linux_10Mb.txt
####################### START M3COPY 20M #################
read  -p "M3COPY 20M Enter para continuar... "
dmesg -c  >>/lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/commands/demonize/demonize -l $lcl $lcl $dcid 21 0 "/home/MoL_Module/mol-ipc/commands/m3copy/m3copy -g 20Mb.txt /dev.ro/shm/linux_20Mb.txt" > /lib/init/rw/m3copy$lcl.txt 2> /lib/init/rw/m3copy$lcl.txt &
sleep 15
cat /proc/dvs/DC$dcid/procs
#rm /dev.ro/shm/linux_20Mb.txt
####################### START M3COPY 40M #################
read  -p "M3COPY 40M Enter para continuar... "
dmesg -c  >>/lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/commands/demonize/demonize -l $lcl $lcl $dcid 21 0 "/home/MoL_Module/mol-ipc/commands/m3copy/m3copy -g 40Mb.txt /dev.ro/shm/linux_40Mb.txt" > /lib/init/rw/m3copy$lcl.txt 2> /lib/init/rw/m3copy$lcl.txt &
sleep 20
cat /proc/dvs/DC$dcid/procs
#rm /dev.ro/shm/linux_40Mb.txt
####################### PRINTING RESUME #################
read  -p "PRINTING RESUME enter para continuar... "
grep t_total /lib/init/rw/rs$lcl.txt > /lib/init/rw/m3copy_results.txt
cat /lib/init/rw/m3copy_results.txt
read  -p "DELETING target files: enter para continuar... "
ls -l /dev.ro/shm/
rm /dev.ro/shm/*.txt
################## FATFS FUSE TESTS #####################
read  -p "FATFS FUSE Enter para continuar... "
mkdir /tmp/fu
dmesg -c  >>/lib/init/rw/dmesg$lcl.txt
/home/MoL_Module/mol-ipc/fatFuse/fatfs_fuse -d -f /tmp/fu > /lib/init/rw/fatfs_fuse$lcl.txt 2> /lib/init/rw/fatfs_fuse_err$lcl.txt &
sleep 5
cat /proc/dvs/DC$dcid/procs
####################### START COPY 10M #################
read  -p "COPY 10M Enter para continuar... "
time cp /tmp/fu/10Mb.txt /dev.ro/shm/ > cp_results.txt 
sleep 5
####################### START COPY 20M #################
read  -p "COPY 20M Enter para continuar... "
time cp /tmp/fu/20Mb.txt /dev.ro/shm/ > cp_results.txt 
sleep 5
####################### START COPY 30M #################
read  -p "COPY 30M Enter para continuar... "
time cp /tmp/fu/30Mb.txt /dev.ro/shm/ > cp_results.txt 
sleep 5
####################### PRINTING RESUME #################
read  -p "PRINTING RESUME enter para continuar... "
cat /lib/init/rw/cp_results.txt
read  -p "DELETING target files: enter para continuar... "
ls -l /dev.ro/shm/
rm /dev.ro/shm/*.txt
exit


