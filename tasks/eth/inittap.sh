#!/bin/bash
#		Topology:
#							NODE 0		
#			tap0 -----br0--linux_routing----eth0 
#			tap1-------|		
ipbr0="192.168.1.20"
iptap0="192.168.1.200"
iptap1="192.168.1.201"
netmask="255.255.255.0"
mactap0="02:AA:BB:CC:DD:00"
mactap1="02:AA:BB:CC:DD:10"
# enable routing between interfaces
echo 1 >  /proc/sys/net/ipv4/ip_forward
# Bridge configuration --------------------------------------------------------
read  -p "Configuring br0. Enter para continuar... "
brctl addbr br0
ifconfig br0 $ipbr0 netmask  $netmask 
ip link set dev br0 up 
# TAP0 configuration --------------------------------------------------------
read  -p "Configuring tap0. Enter para continuar... "
mknod /dev/tap0 c 36 $[ 0 + 16 ]
chmod 666 /dev/tap0
ip tuntap add dev tap0 mode tap
ip link set dev tap0 address $mactap0
ip link set dev tap0 up 
brctl addif br0 tap0
ifconfig tap0 $iptap0 netmask $netmask
ifconfig tap0 | grep addr
# TAP1 configuration --------------------------------------------------------
read  -p "Configuring tap1. Enter para continuar... "
mknod /dev/tap1 c 36 $[ 1 + 16 ]
chmod 666 /dev/tap1
ip tuntap add dev tap1 mode tap
ip link set dev tap1 address $mactap1
ip link set dev tap1 up 
brctl addif br0 tap1
ifconfig tap1  $iptap1 netmask  $netmask
ifconfig tap1 | grep addr
# Link ETH0 to BRIDGE
read  -p "Conecting eth0 to br0. Enter para continuar... "
brctl addif br0 eth0
brctl show
brctl showmacs br0
exit

















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
#read  -p "Configuring tap1. Enter para continuar... "
# add new interface tap1
#ip tuntap add dev tap1 mode tap
# enable tap1
#ip link set dev tap1 up
# link tap interface to bridge
#brctl addif br0 tap1
read  -p "Running lwip-tap on tap0.Enter para continuar... "
./lwip-tap -H -M rmt_nodeid=$rmt -i name=tap0,addr=172.16.$1.2,netmask=255.255.255.0,gw=172.16.$1.1  > stdout.txt 2> stderr.txt &
#read  -p "Running lwip-tap on tap1.Enter para continuar... "
#./lwip-tap -H -M -i name=tap1,addr=172.16.$1.3,netmask=255.255.255.0,gw=172.16.$1.1 &
# route the other node tap interfaces
read  -p "Adding new route. Enter para continuar... "
route add -net 172.16.$rmt.0 gw node$rmt netmask 255.255.255.0
brctl show
exit
