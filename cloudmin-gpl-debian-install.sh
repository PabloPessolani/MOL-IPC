#!/bin/sh
# cloudmin-gpl-debian-install.sh
# Copyright 2005-2010 Virtualmin, Inc.
#
# Installs Cloudmin GPL for Xen and all dependencies on a Debian or
# Ubuntu system

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

# Ask the user first
cat <<EOF
***********************************************************************
*     Welcome to the Cloudmin GPL for Xen installer, version $VER     *
***********************************************************************

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

# Cleanup old repo files
grep -v cloudmin.virtualmin.com /etc/apt/sources.list >/etc/apt/sources.list.clean
mv /etc/apt/sources.list.clean /etc/apt/sources.list

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
apt-get -y install wget
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
if [ "$?" != 0 ]; then
	echo .. import failed
	exit 1
fi
echo .. done
echo ""

# Setup the APT sources file
echo Creating APT repository for Cloudmin packages ..
cat >>/etc/apt/sources.list <<EOF
deb http://cloudmin.virtualmin.com/gpl/debian binary/
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

# APT install Perl, modules and other dependencies
echo Installing required Perl modules using APT ..
uname=`uname -m`
if [ "$uname" = "x86_64" ]; then
  arch=amd64
else
  arch=i386
fi
apt-get -y install perl openssl libio-pty-perl libio-stty-perl libnet-ssleay-perl libwww-perl libdigest-hmac-perl libxml-simple-perl libcrypt-ssleay-perl libauthen-pam-perl cron bind9 xen-tools openssh-client openssh-server
if [ "$?" != 0 ]; then
	echo .. install failed
	exit 1
fi
apt-get -y install libc6-xen	# May fail on 64-bit, but that's OK
apt-get -y install bridge-utils parted
apt-get -y install libdigest-sha1-perl
apt-get -y install `apt-cache search 'ubuntu-xen-server|xen-linux-system' | awk '{ print $1 }' | tail -1`
apt-get -y install xen-utils-3.2 || apt-get -y install xen-utils-4.0 || apt-get -y install xen-utils
if [ "$?" != 0 ]; then
	echo .. install failed
	exit 1
fi
apt-get -y install xen-hypervisor-3.2-1-$arch || apt-get -y install xen-hypervisor-3.2 || apt-get -y install xen-hypervisor-4.0-$arch || apt-get -y install xen-hypervisor
if [ "$?" != 0 ]; then
	echo .. install failed
	exit 1
fi
apt-get install -y libjson-perl
apt-get install -y bind9utils
apt-get install -y dhcp3-server
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
  echo xen_zone=$zone >>/etc/webmin/server-manager/config
  echo xen_zone=$zone >>/etc/webmin/server-manager/this
  echo .. done
fi
echo ""

# Use Xen kernel
echo Configuring GRUB to boot Xen-capable kernel ..
/usr/share/webmin/server-manager/setup-xen-kernel.pl
if [ "$?" != 0 ]; then
  echo .. failed
else
  echo .. done
fi
echo ""

# Open Webmin firewall port
echo Opening port 10000 on host firewall ..
/usr/share/webmin/firewall/open-ports.pl 10000 10001 10002 10003 10004 10005 843
if [ "$?" != 0 ]; then
  echo .. failed
else
  echo .. done
fi
echo ""

# Tell user
hostname=`hostname`
echo Cloudmin GPL has been successfully installed. However, you will need to
echo reboot to activate the new Xen-capable kernel before any Xen instances
echo can be created.
echo
echo One this is done, you can login to Cloudmin at :
echo https://$hostname:10000/

# All done!

