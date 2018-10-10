#!/bin/bash
source ../../../prefix.sh
SvrdName='UserSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"