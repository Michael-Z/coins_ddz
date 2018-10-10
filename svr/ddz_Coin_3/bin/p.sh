#!/bin/bash
source ../../../prefix.sh
SvrdName='DdzChuangGuanSvrd'
ps -ef | grep ${Prefix}${SvrdName} | grep -v "grep"
