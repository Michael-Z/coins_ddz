#!/bin/sh
source ./prefix.sh
ps -ef | grep ${Prefix} | grep -v "grep"