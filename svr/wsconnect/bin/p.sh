#!/bin/bash
source ../../../prefix.sh
SvrdName='WSSConnectSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
