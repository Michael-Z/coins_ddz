#!/bin/bash

help_info(){
	echo "./sigsender.sh SIG pid [pid]"
}

if [ $# -lt 1 ]
then 
	help_info
fi
singal='SIGRTMIN'
case $1 in
	reload) #echo "reload"
	signal='SIGRTMIN'	
	;;
	retire) #echo "retire"
	signal='SIGRTMIN+4'
	;;
	logon) #echo "logon"
	signal='SIGRTMIN+2'
	;;
	logoff) #echo "logoff"
	signal='SIGRTMIN+3'
	;;
	*) echo "invalid singal input:retire logon logoff"
	exit
	;;
esac

shift
	
#echo $signal
for arg in $*
do
	echo "kill $signal $arg"
	kill -$signal $arg
done
