#!/bin/bash
source ../../../prefix.sh
SvrdName='LogSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
