#!/bin/sh
# cloudmin-kvm-debian-install.sh
# Copyright 2005-2011 Virtualmin, Inc.
#
# Installs Cloudmin GPL for KVM and all dependencies on a Debian or Ubuntu
# system.

VER=1.1

# Define functions
yesno () {
	while read line; do
		case $line in
			y|Y|Yes|YES|yes|yES|yEs|YeS|yeS) return 0
			;;
			n|N|No|NO|no|nO) return 1
			;;
			*)
			printf "\nPlease enter y or n: "
			;;
		esac
	done
}

# Show help page
if [ "$1" = "--help" ]; then
	echo $0 [--qemu] [--no-bridge | --bridge-interface ethN]
	exit 0
fi

# Ask the user first
cat <<EOF
*******************************************************************************
*         Welcome to the Cloudmin GPL for KVM installer, version $VER         *
*******************************************************************************

 Operating systems supported by this installer are:

 Debian 4.0 or later on i386 and x86_64
 Ubuntu 8.04 or later on i386 and x86_64

 If your OS is not listed above, this script will fail (and attempting
 to run it on an unsupported OS is not recommended, or...supported).
EOF
printf " Continue? (y/n) "
if ! yesno
then exit
fi
echo ""

# Check for non-kernel mode flag
if [ "$1" = "--qemu" ]; then
	qemu=1
fi

# Check for flag to disable bridge setup
if [ "$1" = "--no-bridge" ]; then
	shift
	nobridge=1
fi

# Check for flag for network interface
if [ "$1" = "--bridge-interface" ]; then
	shift
	interface=$1
	shift
fi

# Cleanup old repo files
grep -v cloudmin.virtualmin.com /etc/apt/sources.list >/etc/apt/sources.list.clean
mv /etc/apt/sources.list.clean /etc/apt/sources.list

# Check for KVM-capable CPU
if [ "$qemu" != 1 ]; then
	echo Checking for hardware support for KVM ..
	grep -e vmx -e svm /proc/cpuinfo >/dev/null
	if [ $? != 0 ]; then
		echo .. not found. Make sure your CPU has Intel VT-x or AMD-V support
		echo ""
		exit 1
	fi
	echo .. found OK
	echo ""
fi

# Check for apt-get
echo Checking for apt-get ..
if [ ! -x /usr/bin/apt-get ]; then
	echo .. not installed. The Cloudmin installer requires APT to download packages
	echo ""
	exit 1
fi
echo .. found OK
echo ""

# Make sure we have wget
echo "Installing wget .."
yum install -y wget
echo ".. done"
echo ""

# Check for wget or curl
echo "Checking for curl or wget..."
if [ -x "/usr/bin/curl" ]; then
	download="/usr/bin/curl -s "
elif [ -x "/usr/bin/wget" ]; then
	download="/usr/bin/wget -nv -O -"
else
	echo "No web download program available: Please install curl or wget"
	echo "and try again."
	exit 1
fi
echo "found $download"
echo ""

# Create Cloudmin licence file
echo Creating Cloudmin licence file
cat >/etc/server-manager-license <<EOF
SerialNumber=GPL
LicenseKey=GPL
EOF
chmod 600 /etc/server-manager-license

# Download GPG keys
echo Downloading GPG keys for packages ..
$download "http://software.virtualmin.com/lib/RPM-GPG-KEY-virtualmin" >/tmp/RPM-GPG-KEY-virtualmin
if [ "$?" != 0 ]; then
	echo .. download failed
	exit 1
fi
$download "http://software.virtualmin.com/lib/RPM-GPG-KEY-webmin" >/tmp/RPM-GPG-KEY-webmin
if [ "$?" != 0 ]; then
	echo .. download failed
	exit 1
fi
echo .. done
echo ""

# Import keys
echo Importing GPG keys ..
apt-key add /tmp/RPM-GPG-KEY-virtualmin && apt-key add /tmp/RPM-GPG-KEY-webmin
echo .. done
echo ""

# Setup the APT sources file
echo Creating APT repository for Cloudmin packages ..
cat >>/etc/apt/sources.list <<EOF
deb http://cloudmin.virtualmin.com/kvm/debian binary/
EOF
apt-get update
echo .. done
echo ""

# Turn off apparmor
if [ -r /etc/init.d/apparmor ]; then
  echo Turning off apparmor ..
  /etc/init.d/apparmor stop
  update-rc.d -f apparmor remove
  echo .. done
  echo ""
fi

# YUM install Perl, modules and other dependencies
echo Installing required Perl modules using APT ..
apt-get -y install perl openssl libio-pty-perl libio-stty-perl libnet-ssleay-perl libwww-perl libdigest-hmac-perl libxml-simple-perl libcrypt-ssleay-perl libauthen-pam-perl cron bind9 lsof parted openssh-client openssh-server
if [ "$?" != 0 ]; then
	echo .. install failed
	exit 1
fi
apt-get -y install kvm qemu
if [ "$?" != 0 ]; then
	echo .. install failed
	exit 1
fi
apt-get install -y libjson-perl
apt-get install -y bind9utils
apt-get install -y dhcp3-server
apt-get install -y libdigest-sha1-perl
apt-get install -y ebtables
echo .. done
echo ""

# Activate cgroups
echo Installing and activating cgroups ..
apt-get install -y cgroup-bin
if [ "$?" != 0 ]; then
	echo .. install failed - CPU limits will not be available
else
	/etc/init.d/cgconfig start
	update-rc.d cgconfig defaults
	echo .. done
fi
echo ""

# Activate KVM kernel module
if [ "$qemu" != 1 ]; then
	echo Activating KVM kernel module ..
	modprobe kvm-intel || modprobe kvm-amd
	/sbin/lsmod | grep kvm >/dev/null
	if [ "$?" != 0 ]; then
		echo .. kernel module did not load successfully
		exit 1
	fi
	sleep 5
	if [ ! -e /dev/kvm ]; then
		echo .. KVM device file /dev/kvm is missing
		exit 1
	fi
	echo .. done
	echo ""
fi

# Create loop devices if missing
echo Creating /dev/loop devices ..
for loop in 0 1 2 3 4 5 6 7; do
	if [ ! -r "/dev/loop$loop" ]; then
		mknod "/dev/loop$loop" b 7 $loop
	fi
done
echo .. done
echo ""

# APT install webmin, theme and Cloudmin
echo Installing Cloudmin packages using APT ..
apt-get -y install webmin
apt-get -y install webmin-server-manager webmin-virtual-server-theme webmin-virtual-server-mobile webmin-security-updates
if [ "$?" != 0 ]; then
        echo .. install failed
        exit 1
fi
mkdir -p /xen
echo .. done
echo ""

# Configure Webmin to use theme
echo Configuring Webmin ..
grep -v "^preroot=" /etc/webmin/miniserv.conf >/tmp/miniserv.conf.$$
echo preroot=authentic-theme >>/tmp/miniserv.conf.$$
cat /tmp/miniserv.conf.$$ >/etc/webmin/miniserv.conf
rm -f /tmp/miniserv.conf.$$
grep -v "^theme=" /etc/webmin/config >/tmp/config.$$
echo theme=authentic-theme >>/tmp/config.$$
cat /tmp/config.$$ >/etc/webmin/config
rm -f /tmp/config.$$
/etc/webmin/restart
echo .. done
echo ""

# Setup BIND zone for virtual systems
basezone=`hostname -d`
if [ "$basezone" = "" ]; then
	basezone=example.com
fi
zone="cloudmin.$basezone"
echo Creating DNS zone $zone ..
/usr/share/webmin/server-manager/setup-bind-zone.pl --zone $zone --auto-view
if [ "$?" != 0 ]; then
  echo .. failed
else
  echo kvm_zone=$zone >>/etc/webmin/server-manager/config
  echo kvm_zone=$zone >>/etc/webmin/server-manager/this
  echo .. done
fi
echo ""

# Set Qemu mode flag
if [ "$qemu" = 1 ]; then
	echo kvm_qemu=1 >>/etc/webmin/server-manager/config
fi

# Enable bridge
if [ "$nobridge" = "" ]; then
	echo Creating network bridge ..
	/usr/share/webmin/server-manager/setup-kvm-bridge.pl $interface
	brex=$?
	if [ "$brex" = 0 ]; then
		echo .. already configured
	else
		if [ "$brex" = 1 ]; then
			echo .. done
		else
			echo .. bridge creation failed
		fi
	fi
	echo ""
fi

# Open Webmin firewall port
echo Opening port 10000 on host firewall ..
/usr/share/webmin/firewall/open-ports.pl 10000 10001 10002 10003 10004 10005 843
if [ "$?" != 0 ]; then
  echo .. failed
else
  echo .. done
fi
echo ""

# Tell user about need to reboot
hostname=`hostname`
if [ "$brex" = 1 ]; then
	echo Cloudmin GPL has been successfully installed. However, you will
	echo need to reboot to activate the network bridge before any KVM
	echo instances can be created.
	echo
	echo One this is done, you can login to Cloudmin at :
	echo https://$hostname:10000/
else
	echo Cloudmin GPL has been successfully installed.
	echo
	echo You can login to Cloudmin at :
	echo https://$hostname:10000/
fi

# All done!
