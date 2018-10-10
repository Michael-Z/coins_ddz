#!/bin/bash

function usage
{
  echo "./publish [svrname]"
  exit
}

PATH="$(dirname $0)/lbf/lbf:$PATH"

[ "$1" = "" ] && usage

echo "please select which to pack: "
echo "1)  connect"
echo "2)  dbproxy"
echo "5)  gm"
echo "6)  hall"
echo "7)  log"
echo "8)  room"
echo "9)  routing"
echo "10)  user"
echo "17) *scripts"
echo "18) *configs"
echo "19) lib"
echo "all) all"

read -p "please select which to pack: " var_select_list

var_pack_list=""

for _var_select in ${var_select_list}; do
  case ${_var_select} in 
    1)  var_pack_list+=" ./svr/connect/bin/" 
        ;;
    2)  var_pack_list+=" ./svr/dbproxy/bin/" 
        ;;
    5)  var_pack_list+=" ./svr/gm/bin/"
        ;;
    6)  var_pack_list+=" ./svr/hall/bin/"
        ;;
    7)  var_pack_list+=" ./svr/log/bin/"
		var_pack_list+=" ./svr/log/db/"
        ;;
    8)  var_pack_list+=" ./svr/room/bin/"
        ;;
    9)  var_pack_list+=" ./svr/routing/bin/"
        ;;
    10) var_pack_list+=" ./svr/user/bin/"
        ;;
	11) var_pack_list+=" ./svr/teabar/bin/"
		var_pack_list+=" ./svr/teabar/conf/"
        ;;
	12) var_pack_list+=" ./svr/sdr_xx96/bin/"
        ;;
    17) var_pack_list+=" *.sh"
		var_pack_list+=" ./svr/connect/bin/*.sh"
		var_pack_list+=" ./svr/hall/bin/*.sh"
		var_pack_list+=" ./svr/room/bin/*.sh"
		var_pack_list+=" ./svr/game/bin/*.sh"
		var_pack_list+=" ./svr/user/bin/*.sh"
		var_pack_list+=" ./svr/routing/bin/*.sh"
		var_pack_list+=" ./svr/dbproxy/bin/*.sh"
		var_pack_list+=" ./svr/*.sh"
        ;;
    18) var_pack_list+=" ./svr/conf/"
        ;;
	all) var_pack_list+=" ./svr/connect/bin/" 
		 var_pack_list+=" ./svr/dbproxy/bin/" 
		 var_pack_list+=" ./svr/gm/bin/"
		 var_pack_list+=" ./svr/hall/bin/"
		 var_pack_list+=" ./svr/log/bin/"
		 var_pack_list+=" ./svr/log/db/"
		 var_pack_list+=" ./svr/room/bin/"
		 var_pack_list+=" ./svr/routing/bin/"
		 var_pack_list+=" ./svr/user/bin/"
		 var_pack_list+=" ./svr/teabar/bin/"
		 var_pack_list+=" ./svr/teabar/conf/"
		 var_pack_list+=" ./svr/sdr_xx96/bin/"
		 var_pack_list+=" *.sh"
		 var_pack_list+=" ./svr/*.sh"
		 var_pack_list+=" ./svr/conf/"
        ;;
    *)  echo "unknown tpye ${_var_select}"; exit 0;;
  esac
done


echo "================="
echo "begin pack"
var_dir="."
var_file="${var_dir}/$1_$(date '+%Y%m%d%H%M%S').tar.gz"
tar -zcvf ${var_file} ${var_pack_list} --exclude \*.svn\* --exclude \*.log\* --exclude \*.tar.gz\*

echo "================="
echo "calc md5"
md5sum ${var_file}

echo "================="
# vim:ts=2:sw=2:et:
