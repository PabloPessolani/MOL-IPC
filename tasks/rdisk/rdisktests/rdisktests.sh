#!/bin/bash

dcid=0

###################### TEST RDISK NODE0 #################
read  -p "TEST RDISK Enter para continuar... "
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
cd /home/MoL_Module/mol-ipc/commands/demonize
#demonize -{l|r|b} <dcid> <endpoint> <pid> <filename> <args...> 
./demonize -l $dcid 31 0 "/home/MoL_Module/mol-ipc/tasks/rdisk/rdisktests/01_test_devopen 0" > 01_test_devopen.txt 2> 01_test_devopenerr.txt 
dmesg -c  >> /home/MoL_Module/mol-ipc/dmesg$lcl.txt
sleep 2
cat /proc/drvs/VM$dcid/procs
exit
