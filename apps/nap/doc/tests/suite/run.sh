#!/bin/bash
#
# Author: Sebastian Robitzsch
#
# apt install r-base iperf 
#
# Prerequisites
# - add test.point to /etc/hosts or to your DNS server
# - NAP must be the GW and on a.b.c.1
# - r-base-core is installed on the machine where this script is exectued
# - iperf is installed on the NAP, the test.point and from the machine where
#   this script is executed
# - web server is installed on $FQDN and a file called 'file' is placed 
#   into the root HTML directory

INT=$1
DURATION=$2
USER="user"
FQDN="test.point"

function analyseIperf()
{
	cd $DIR_IPERF
	echo "Cleaning $FILE"
	../../cleanUpIperf.awk $FILE
	echo "R --no-restore --no-save --silent --args $FILE.cleaned $DURATION < ../../plotIperf.R"
	R --no-restore --no-save --args $FILE.cleaned $DURATION < ../../plotIperf.R > /dev/null
	cd ../../
}

function analysePing()
{
	cd $DIR_PING
	echo "Cleaning $FILE"
	../../cleanUpPing.awk $FILE
	echo "R --no-restore --no-save --silent --args $FILE.cleaned $DURATION < ../../plotPing.R"
	R --no-restore --no-save --args $FILE.cleaned $DURATION < ../../plotPing.R > /dev/null
	cd ../../
}

function runIperfUdp()
{
	# Start IPERF server
	echo "Start UDP IPERF server on $IPERF_SERVER"
	HOST=$IPERF_SERVER
	ssh $USER@$HOST 'iperf -s -u -i 1 > /home/'$USER'/'$FILE' 2>&1 &'

	# Start IPERF client
	echo "Start UDP IPERF client on $IPERF_CLIENT"
	HOST=$IPERF_CLIENT
	ssh $USER@$HOST 'iperf -c '$IPERF_SERVER' -u -b 1G -i 1 -t '$DURATION' > /dev/null &'
	sleep $DURATION
	sleep 3
	# Now kill the server application and transfer the file over here
	HOST=$IPERF_SERVER
	ssh $USER@$HOST 'pkill -15 iperf'
	HOST=$IPERF_CLIENT
	ssh $USER@$HOST 'pkill -15 iperf'
	HOST=$IPERF_SERVER
	DIR=$DIR_IPERF
	runScp
}

function runIperfTcp()
{
	# Start IPERF server
	echo "Start UDP IPERF server on $IPERF_SERVER"
	HOST=$IPERF_SERVER
	ssh $USER@$HOST 'iperf -s -i 1 > /home/'$USER'/'$FILE' 2>&1 &'

	# Start IPERF client
	echo "Start UDP IPERF client on $IPERF_CLIENT"
	HOST=$IPERF_CLIENT
	ssh $USER@$HOST 'iperf -c '$IPERF_SERVER' -i 1 -t '$DURATION' > /dev/null &'
	sleep $DURATION
	sleep 3
	# Now kill the server application and transfer the file over here
	HOST=$IPERF_SERVER
	ssh $USER@$HOST 'pkill -15 iperf'
	HOST=$IPERF_CLIENT
	ssh $USER@$HOST 'pkill -15 iperf'
	HOST=$IPERF_SERVER
	DIR=$DIR_IPERF
	runScp
}

function runScp()
{
	echo "Moving $FILE from $HOST to here"
	scp $USER@$HOST:/home/$USER/$FILE $DIR > /dev/null
	ssh $USER@$HOST 'rm /home/'$USER'/'$FILE > /dev/null
}

################################################################################
if [ $# -ne 2 ]
then
	echo "Provide the interface on which the client communicates with the \
NAP and the duration in [s] for each test"
	echo "For instance, ./run.sh enp3s0 60"
	exit 0
fi

echo "Reading IP address of $INT"
MY_IP=`ip a show $INT | grep "inet " | awk '{split($2, ip, "/"); print ip[1]}'`
echo "IP address: $MY_IP"
NAP_IP=`echo $MY_IP | awk '{split($0, ip, "."); print ip[1]"."ip[2]"."ip[3]".1"}'`

################################################################################
# testing SSH into localhost, NAP and $FQDN
echo "Testing SSH command to localhost. If password prompt appears, please type it"
ssh $USER@localhost 'echo "Hello from $HOSTNAME"'
echo "Testing connection to NAP on $NAP_IP"
ping $NAP_IP -c 1 -W 3 > /dev/null

if [ $? -ne 0 ]; then
	echo "IP assignment failed ... check your adapter settings"
	exit 1
fi

echo "Testing SSH command to NAP. If password prompt appears, please type it"
ssh $USER@$NAP_IP 'echo "Hello from $HOSTNAME"'
echo "Testing SSH command to $FQDN. If password prompt appears, please type it"
ssh $USER@$FQDN 'echo "Hello from $HOSTNAME"'
################################################################################
# Creating directories
TIME=`date +%H-%M`
DIR_LOC="results-$TIME"
DIR_IPERF=$DIR_LOC"/iperf"
DIR_WGET=$DIR_LOC"/wget"
DIR_PING=$DIR_LOC"/ping"

echo "Creating directories"
mkdir -p $DIR_LOC
mkdir -p $DIR_IPERF
mkdir -p $DIR_WGET
mkdir -p $DIR_PING

################################################################################
# Ping to NAP
echo "*************************************************************************"
echo "Starting ping test to NAP on $NAP_IP for $DURATION iterations"
FILE=$TIME"nap"
ping $NAP_IP -c $DURATION > $DIR_PING/$FILE
analysePing

################################################################################
# Ping to $FQDN
echo "*************************************************************************"
echo "Starting ping test to $FQDN for $DURATION iterations"
FILE=$TIME$FQDN	
ping $FQDN -c $DURATION > $DIR_PING/$FILE 2>&1
analysePing

################################################################################
# Testing iperf UDP to NAP
echo "*************************************************************************"
echo "Starting IPERF-UDP to NAP ($NAP_IP) for $DURATION seconds"
FILE=$TIME"nap.udp"
IPERF_SERVER=$NAP_IP
IPERF_CLIENT="localhost"
runIperfUdp
analyseIperf

################################################################################
# Testing iperf TCP to NAP
echo "*************************************************************************"
echo "Starting IPERF-TCP to NAP ($NAP_IP) for $DURATION seconds"
FILE=$TIME"nap.tcp"
IPERF_SERVER=$NAP_IP
IPERF_CLIENT="localhost"
runIperfTcp
analyseIperf

################################################################################
################################################################################
################################################################################
# getting IP for $FQDN - iperf doesn't like FQDNs
FQDN_IP=`ping -c 1 $FQDN | awk '{if ($0 ~ /PING/){split($3, tmp, "("); split(tmp[2], ip, ")"); print ip[1]}}'`

################################################################################
# Testing iperf UDP to test.flips
echo "*************************************************************************"
echo "IPERF-UDP to $FQDN ($FQDN_IP) for $DURATION seconds"
FILE=$TIME$FQDN".udp"
IPERF_SERVER=$FQDN_IP
IPERF_CLIENT="localhost"
runIperfUdp
analyseIperf

################################################################################
# Testing iperf TCP to test.flips
echo "*************************************************************************"
echo "IPERF-TCP to $FQDN ($FQDN_IP) for $DURATION seconds"
FILE=$TIME$FQDN".tcp"
IPERF_SERVER=$FQDN_IP
IPERF_CLIENT="localhost"
runIperfTcp
analyseIperf

# Testing wget
echo "*************************************************************************"
echo "Starting HTTP progressive download of file 'file' from test.point"
FILE=$TIME"wget"
wget --delete-after $FQDN/file --read-timeout 5s --tries=1 > $DIR_WGET/$FILE 2>&1

echo "*************************************************************************"
echo "All done. Check '$DIR_LOC' for the results"
