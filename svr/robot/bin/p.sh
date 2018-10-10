#!/bin/bash
source ../../../prefix.sh
SvrdName='TeaBarSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"