#!/bin/sh

dir=`pwd`

echo "please select which to start: "

group=$1

read -p "please select which to pack: " var_select_list

start(){
	if [ $#==2 ] 
	then
		svrindex=$2
	else
		svrindex=""
	fi
	cd $dir/svr$svrindex/$1/bin
	echo "starting $1 in " .. `pwd`
	./start.sh
}

for _var_select in ${var_select_list}; do
	case ${_var_select} in
		route) start "routing" 
			;;
		connect) start "connect" 
			;;
		dbproxy) start "dbproxy" 
			;;
		game) start "game" 
			;;
		hall) start "hall" 
			;;
		log) start "log" 
			;;
		room) start "room" 
			;;
		user) start "user" 
			;;
		fpf) start "fpf" 
			;;
		gm) start "gm" 
			;;
		all) start "routing"; 
			 start "dbproxy";
			 start "log";
			 start "connect";
			 start "user";
			 start "hall";
			 start "room";
			 start "teabar";
			start "ddz_es";
			start "ddz_jd";
			start "ddz_mc";
			start "ddz_tj";
			start "ddz_yc";
			start "gzp_tc";
			start "mj_bd";
			start "mj_es";
			start "mj_minxian";
			start "pdk_classic";
			start "pdk_xf";
			start "pk_hcng";
			start "sdr_3jing";
			start "sdr_5jing";
			start "sdr_bd";
			start "sdr_bd_new";
			start "sdr_bh";
			start "sdr_ch";
			start "sdr_dyfj";
			start "sdr_es";
			start "sdr_es_match";
			start "sdr_js";
			start "sdr_lc";
			start "sdr_lf";
			start "sdr_xcch";
			start "sdr_xe";
			start "sdr_xe96";
			start "sdr_xf";
			start "sdr_xx96";
			start "sdr_yc";
			start "sdr_yc_2";
			start "sdr_yclj";
			start "sdr_ysg";
			start "ssp_lj";
			start "ssp_ly";
             ;;
	*) echo "unknown type ${_var_select}"; exit 0;;
	esac
done


