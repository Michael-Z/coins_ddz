#!/bin/sh
source ../../../prefix.sh
source ../../prefix.sh
SvrdName='DdzChuangGuanSvrd'
BIN=${Prefix}${SvrdName}
BACKGOURD=1
PORT=$[11212+${PortPtr}+${SPtr}]
IP=::ffff:0.0.0.0
SVID=$[1+${SPtr}]
STATE='us:0000,ts:0000,r:F'
./$BIN -d $BACKGOURD -p $PORT -s $SVID -h $IP -t $STATE
