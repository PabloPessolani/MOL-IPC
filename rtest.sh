#!/bin/bash
if [ $# -ne 2 ]
then 
	echo "usage: $0 <proxy_type> <lcl_nodeid>"
	echo "proxy_type:"
	echo "u: udp proxies "
	echo "U: UDT proxies "
	echo "t: tcp proxies "
	echo "h: tcp threaded proxies "
	echo "T: TIPC proxies "
	echo "S: Server TCP proxy"
	echo "C: Client TCP proxy"
	echo "z: tcp threaded proxies with LZ4 data compression"
	echo "Z: TIPC proxies with LZ4 data compression"
	echo "s: Socat proxies"
	echo "l: LWIP proxies"
	echo "R: RAW proxies"
	echo "lcl_nodeid of this node" 
	exit 1 
fi

let lcl_nodeid=$2
echo "lcl_nodeid=$lcl_nodeid"

if [ $lcl_nodeid -gt 31 ]
then 
	echo "lcl_nodeid must be < 31"
	exit 1
fi
if [ $lcl_nodeid -lt 0 ]
then 
	echo "lcl_nodeid must be > 0"
	exit 1
fi

dmesg -c > /dev/null
if [ $1 == "u" ]
then 
	echo "UDP proxies selected"
elif  [ $1 == "U" ]
then
	echo "UDT proxies selected"
elif  [ $1 == "t" ]
then
	echo "TCP proxies selected"
elif  [ $1 == "h" ]
then
	echo "TCP threaded proxies selected"
elif  [ $1 == "z" ]
then
	echo "tcp threaded proxies with LZ4 data compression selected"
elif  [ $1 == "Z" ]
then
	echo "TIPC proxies with LZ4 data compression selected"
elif  [ $1 == "T" ]
then
	echo "TIPC proxies selected"
elif  [ $1 == "S" ]
then
	echo "Server TCP proxy selected"
elif  [ $1 == "C" ]
then
	echo "Client TCP proxy selected"
elif  [ $1 == "s" ]
then
	echo "Socat proxies selected"
elif  [ $1 == "l" ]
then
	echo "LWIP proxies selected"
elif  [ $1 == "R" ]
then
	echo "RAW proxies selected"
else
	echo "usage: $0 <proxy_type> <lcl_nodeid>"
	echo "proxy_type:"
	echo "u: udp"
	echo "U: UDT proxies "
	echo "t: tcp"
	echo "h: tcp threaded proxies "
	echo "T: TIPC"
	echo "S: Server TCP proxy"
	echo "C: Client TCP proxy"
	echo "z: tcp threaded proxies with LZ4 data compression"
	echo "Z: TIPC proxies with LZ4 data compression"
	echo "s: Socat proxies"
	echo "l: LWIP proxies"
	echo "R: RAW proxies"
	echo "lcl_nodeid of this node"
	exit 1
fi
/usr/local/sbin/spread  > spread.txt &			
# cd /home/MoL_Module/mol-module
insmod /home/MoL_Module/mol-module/mol_replace.ko
lsmod
#cd /home/MoL_Module/mol-ipc
read  -p "Initialiting DRVS: Enter para continuar... "
/home/MoL_Module/mol-module/mol-ipc/tests/test_dvs_end
/home/MoL_Module/mol-module/mol-ipc/tests/test_dvs_init -n $lcl_nodeid -D 0
read  -p "Initialiting DC0: Enter para continuar... "
/home/MoL_Module/mol-module/mol-ipc/tests/test_dc_init -d 0

let rmt_nodeid=(1 - $lcl_nodeid)
read  -p "Starting proxies to remote node $rmt_nodeid. Enter para continuar... "

if [ $1 == "u" ] 
then 
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy
	/home/MoL_Module/mol-module/mol-ipc/proxy/udp_proxy node$rmt_nodeid $rmt_nodeid >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
elif  [ $1 == "U" ]
then 
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/udt_proxy node$rmt_nodeid $rmt_nodeid >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
elif  [ $1 == "t" ]
then 
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
	echo 1 > /proc/sys/net/ipv4/tcp_low_latency
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/tcp_proxy node$rmt_nodeid $rmt_nodeid >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
elif  [ $1 == "h" ]
then 
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
	echo 1 > /proc/sys/net/ipv4/tcp_low_latency
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/tcp_th_proxy node$rmt_nodeid $rmt_nodeid  >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
elif  [ $1 == "z" ]
then 
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
	echo 1 > /proc/sys/net/ipv4/tcp_low_latency
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/lz4tcp_th_proxy node$rmt_nodeid $rmt_nodeid  >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
elif  [ $1 == "S" ]
then 
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
	echo 1 > /proc/sys/net/ipv4/tcp_low_latency
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/svr_proxy node$rmt_nodeid $rmt_nodeid  >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
elif  [ $1 == "C" ]
then 
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
	echo 1 > /proc/sys/net/ipv4/tcp_low_latency
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/clt_proxy node$rmt_nodeid $rmt_nodeid  >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
elif  [ $1 == "s" ]
then 
#    	PARA DESHABILITAR EL ALGORITMO DE NAGLE!! 
	echo 1 > /proc/sys/net/ipv4/tcp_low_latency
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	RETRIES=5
	INTERVAL=2
	if [ $lcl_nodeid == '0' ]
	then
		LCLNODE=node0
		LCLPORT=3001
		RMTNODE=node1
		RMTPORT=3000
		PXID=1
	else
		LCLNODE=node1
		LCLPORT=3000
		RMTNODE=node0
		RMTPORT=3001
		PXID=0
	fi
	echo $LCLNODE:$LCLPORT $RMTNODE:$RMTPORT  
	socat -u  -T1000 TCP4-LISTEN:$LCLPORT,forever,ignoreeof,rcvlowat=1 FD:1 | ./socat_rproxy  &
#	nc  -l  -p $LCLPORT | ./socat_rproxy  &
	RPID=$!
	read  -p "Enter para continuar... "
	./socat_sproxy $RMTNODE $PXID $RPID | socat -u -d -x -T1000 FD:0 TCP:$RMTNODE:$RMTPORT,forever,ignoreeof,sndtimeo=1 &
#	./socat_sproxy | nc -w 1000 $RMTNODE $RMTPORT  &
	SPID=$!
#	read  -p "Enter para continuar... "
#	sleep 3
#	echo socat_up $RMTNODE $PXID $SPID $RPID
#	./socat_up $RMTNODE $PXID $SPID $RPID
elif  [ $1 == "l" ]
then 
#	cd /home/MoL_Module/lwip-tap
	echo 1 >  /proc/sys/net/ipv4/ip_forward
	# add bridge interface on host
	brctl addbr br0
	# set bridge interface IP address 
	ifconfig br0 172.16.$lcl_nodeid.1 netmask 255.255.255.0 
	read  -p "Configuring tap0. Enter para continuar... "
	# add new interface tap0
	ip tuntap add dev tap0 mode tap
	# enable tap0
	ip link set dev tap0 up 
	# link tap interface to bridge
	brctl addif br0 tap0
	read  -p "Running lwip-tap on tap0.Enter para continuar... "
	/home/MoL_Module/lwip-tap/lwip-tap -H -M rmt_nodeid=$rmt_nodeid -i name=tap0,addr=172.16.$lcl_nodeid.2,netmask=255.255.255.0,gw=172.16.$lcl_nodeid.1  > stdout.txt 2> stderr.txt &
	brctl show
elif  [ $1 == "R" ]
then 
	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	ping -c 2 node$rmt_nodeid
#read  -p "ARRANCANDO  rawmsgq Enter para continuar... "
#	./rawmsgq node$rmt_nodeid $rmt_nodeid eth0 >rawout$rmt_nodeid.txt 2>rawerr$rmt_nodeid.txt &	
read  -p "ARRANCANDO raw_proxy Enter para continuar... "
	./raw_proxy node$rmt_nodeid $rmt_nodeid eth0 > /lib/init/rw/raw_proxyout$rmt_nodeid.txt 2> /lib/init/rw/raw_proxyerr$rmt_nodeid.txt &	
read  -p "DETENER AQUI Enter para continuar... "
elif  [ $1 == "Z" ]
then 
	tipc-config -net=4711 -a=1.1.10$lcl_nodeid -be=eth:eth0
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/lz4tipc_proxy node$rmt_nodeid $rmt_nodeid >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &	
else 
#  TIPC PROXIES
	tipc-config -net=4711 -a=1.1.10$lcl_nodeid -be=eth:eth0
#	cd /home/MoL_Module/mol-module/mol-ipc/proxy/
	/home/MoL_Module/mol-module/mol-ipc/proxy/tipc_proxy node$rmt_nodeid $rmt_nodeid >node$rmt_nodeid.txt 2>error$rmt_nodeid.txt &
fi
read  -p "Adding new route. Enter para continuar... "
route add -net 172.16.$rmt_nodeid.0 netmask 255.255.255.0 gw 192.168.1.10$rmt_nodeid 
sleep 2
read  -p "Enter para continuar... "
cat /proc/dvs/nodes
cat /proc/dvs/proxies/info
cat /proc/dvs/proxies/procs
read  -p "Adding $rmt_nodeid to DC0. Enter para continuar..."
/home/MoL_Module/mol-module/mol-ipc/tests/test_add_node 0 $rmt_nodeid
exit







