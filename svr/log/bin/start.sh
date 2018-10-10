#!/bin/sh
source ../../../prefix.sh
source ../../prefix.sh
SvrdName='LogSvrd'
BIN=${Prefix}${SvrdName}
BACKGOURD=1
PORT=$[9712+${PortPtr}+${SPtr}]
IP=::ffff:0.0.0.0
SVID=$[1+${SPtr}]
./$BIN -d $BACKGOURD -p $PORT -s $SVID -h $IP
