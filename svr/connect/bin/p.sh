#!/bin/bash
source ../../../prefix.sh
SvrdName='ConnectSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
