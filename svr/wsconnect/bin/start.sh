#!/bin/sh
source ../../../prefix.sh
source ../../prefix.sh
SvrdName='WSSConnectSvrd'
BIN=${Prefix}${SvrdName}
BACKGOURD=1
PORT=$[15012+${PortPtr}+${SPtr}]
IP=::ffff:0.0.0.0
SVID=$[10001+${SPtr}]
./$BIN -d $BACKGOURD -p $PORT -s $SVID -h $IP
