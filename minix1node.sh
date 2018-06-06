#!/bin/bash
dmesg -c
#/home/jara/mol-ipc/tests/test_drvs_end
#/home/jara/mol-ipc/tests/test_drvs_init -n 0 -D 16777215
/usr/local/sbin/spread  > spread.txt &			
#/home/jara/mol-ipc/tests/test_vm_init -v 0
read  -p "SYSTASK Enter para continuar... "
dmesg -c  > /home/jara/mol-ipc/dmesg.txt
cd /home/jara/mol-ipc/tasks/systask
./systask -v 0 -n 1 > systask0.txt &
read  -p "PM Enter para continuar... "
dmesg -c  >> /home/jara/mol-ipc/dmesg.txt
cd /home/jara/mol-ipc/servers/pm
./pm 0 > pm0.txt &
read  -p "INIT Enter para continuar... "
dmesg -c  >> /home/jara/mol-ipc/dmesg.txt
cd /home/jara/mol-ipc/servers/init
./init 0 > init0.txt &
cat /proc/drvs/nodes > minix1node.txt
cat /proc/drvs/proxies/info  >> minix1node.txt
cat /proc/drvs/proxies/procs >> minix1node.txt
cat /proc/drvs/VM0/info >> minix1node.txt
cat /proc/drvs/VM0/procs >> minix1node.txt











