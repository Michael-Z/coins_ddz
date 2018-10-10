#!/bin/bash
source ../../../prefix.sh
SvrdName='RoomSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
