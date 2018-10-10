#!/bin/bash
source ../../../prefix.sh
SvrdName='DBProxySvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
