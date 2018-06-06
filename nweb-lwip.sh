#!/bin/bash
if [ ! -d "/home/nweb" ]; then
  echo "/home/nweb directory do not exist!!"
  exit
fi
if [ ! -d "/lib/init/rw" ]; then
  echo "/lib/init/rw directory do not exist!!"
  exit
fi
if [ ! -f "/home/nweb/index.htm" ]; then
  echo "/home/nweb/index.htm file do not exist!!"
  exit
fi
mkdir /lib/init/rw/nweb
cp /home/nweb/*  /lib/init/rw/nweb
ls -l /lib/init/rw/nweb/
df /lib/init/rw
####################### CONFIG TAP & BR #################
echo 1 >  /proc/sys/net/ipv4/ip_forward
read  -p "Config TAP and BR: Enter para continuar... "
ifconfig | grep tap
mknod /dev/tap0 c 36 $[ 0 + 16 ]
chmod 666 /dev/tap0
ls -l /dev/tap0
# add bridge interface on host
brctl addbr br0
# set bridge interface IP address 
ifconfig br0 172.16.0.3 netmask 255.255.255.0 
read  -p "Configuring tap0. Enter para continuar... "
# add new interface tap0
ip tuntap add dev tap0 mode tap
# enable tap0
ip link set dev tap0 up 
# link tap interface to bridge
brctl addif br0 tap0
####################### START LWIP #################
read  -p "Starting lwip-tap. Enter para continuar... "
/home/MoL_Module/lwip-tap/lwip-tap -n -d -i name=tap0,addr=172.16.0.2,netmask=255.255.255.0,gw=172.16.0.3  > lwipout.txt 2> lwiperr.txt &
ifconfig tap0
echo "the test url is http://172.16.0.2/index.htm" 
exit

