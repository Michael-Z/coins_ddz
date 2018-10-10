#!/bin/bash
source ../../../prefix.sh
SvrdName='GmSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
