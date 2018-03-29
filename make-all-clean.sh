#!/bin/bash
# this is a simple script to recompile everything useful for basic testing
# useful for a quick start or after changing branches
# not a replacement for reading the HowTo!
#set -x
#set -e

DIR=`pwd`

LSB=`which lsb_release`

if [ "$LSB" == "" ]; then
	echo "Cannot find lsb_release binary to determine OS version and package dependencies"
	exit 1
fi

OS=`$LSB -id | awk '{if ($0 ~ /Distributor/) print $3}'`

echo -ne "Installing packages"

if [ "$OS" == "Debian" ]; then
	OS_V=`$LSB -r | awk '{split($2,tmp,"."); print tmp[1]}'`

	if [[ $OS_V -ge 9 ]]; then
		echo -n " for Debian 9"
		sudo apt install `cat apt-get-debian9.txt` --yes --force-yes 2>/dev/null 1>/dev/null
	else
		echo -n " for Debian 8"
		sudo apt install `cat apt-get.txt` --yes --force-yes 2>/dev/null 1>/dev/null
	fi
else
	echo -n " for non Debian OSs (OS = '$OS')"
	sudo apt install `cat apt-get.txt` --yes --force-yes 2>/dev/null 1>/dev/null
fi

if [ $? -ne 0 ]; then
	sudo apt install `cat apt-get.txt` --yes --force-yes
	exit 1
fi

echo -e "\tok"

CORES=`nproc`
RAM=`cat /proc/meminfo | grep MemTotal | awk '{printf ("%.0f\n", $2/1000000)}'`
if [ $RAM -lt $CORES ]; then
	if [ $RAM -eq 0 ]; then
		CORES=1
	else
		CORES=$RAM
	fi
fi

echo -ne "Stop NAP"
sudo pkill -15 nap > /dev/null

if [ $? -ne 0 ]; then
	echo -e "\t\t\t\tnot running"
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Stop MOOSE"
sudo pkill -15 moose > /dev/null

if [ $? -ne 0 ]; then
	echo -e "\t\t\t\tnot running"
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Stop MONA"
sudo pkill -15 mona > /dev/null

if [ $? -ne 0 ]; then
	echo -e "\t\t\t\tnot running"
else
	echo -e "\t\t\t\tok"
fi

cd $DIR/src/
#sudo make uninstall
echo -ne "Run autoconf"
autoconf > /dev/null

if [ $? -ne 0 ]; then
	autoconf
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Configure make"
./configure --disable-linuxmodule > /dev/null

if [ $? -ne 0 ]; then
	./configure --disable-linuxmodule
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Compile ICN core"
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\tok"
fi

echo -ne "Install core"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

# Blackadder library
cd $DIR/lib/
echo -ne "Prepare Blackadder API"
autoreconf -fi 2>/dev/null 1>/dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\tok"
fi

echo -ne "Configure Blackadder API"
./configure > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\tok"
fi

echo -ne "Compile Blackadder API"
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\tok"
fi

echo -ne "Install Blackadder API"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\tok"
fi

## MAPI
cd $DIR/lib/mapi
echo -ne "Compile MAPI"
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Install MAPI"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

# Monitoring framework
cd $DIR/lib/moly
echo -ne "Compile MOLY\t"
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j2 > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\tok"
fi

echo -ne "Install MOLY"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Compile BAMPERS\t"
cd $DIR/lib/bampers
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j2 > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\tok"
fi

echo -ne "Install BAMPERS"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Compile MONA"
cd $DIR/apps/mona
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Install MONA"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

### MOOSE
echo -ne "Compile MOOSE"
cd $DIR/apps/moose
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Install MOOSE"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

### NAP
echo -ne "Compile NAP"
cd $DIR/apps/nap
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

echo -ne "Install NAP"
sudo make install > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

# Topology manager
cd $DIR/TopologyManager/
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

echo -ne "Compile TM"
make > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\t\tok"
fi

# Examples
echo -ne "Compile MOLY Examples\t"
cd $DIR/examples/moly/
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\tok"
fi

cd $DIR/examples/samples
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

echo -ne "Compile traffic enginnering examples"
cd $DIR/examples/traffic_engineering/
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\tok"
fi

echo -ne "Compile bandwidth measurement tool"
./makeBandwidth.sh > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\tok"
fi

echo -ne "Compile video streaming example"
cd $DIR/examples/video_streaming/
make clean > /dev/null

if [ $? -ne 0 ]; then
	exit 1
fi

make -j$CORES > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\tok"
fi

echo -ne "Compile surrogacy example"
cd $DIR/examples/surrogacy
make > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\tok"
fi

cd $DIR/deployment/
echo -ne "Compile deployment tool"
make clean > /dev/null
make > /dev/null

if [ $? -ne 0 ]; then
	exit 1
else
	echo -e "\t\t\tok"
fi

# ICN-SDN app
cd $DIR/apps/icn-sdn
echo -ne "Compile ICN-SDN"
rm messages/{messages.pb.cc,messages.pb.h} > /dev/null
pushd messages > /dev/null
protoc -I=. --cpp_out=. messages.proto > /dev/null
popd > /dev/null
make clean > /dev/null
make > /dev/null

if [ $? -ne 0 ]; then
        exit 1
else
        echo -e "\t\t\t\tok"
fi

