#!/bin/bash
source ../../../prefix.sh
SvrdName='HallSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
