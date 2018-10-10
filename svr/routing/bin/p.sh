#!/bin/bash
source ../../../prefix.sh
SvrdName='RoutingSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
