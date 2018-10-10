#!/bin/sh
#set -e
#set -x

source ./prefix.sh
# 注意: pgrep 后面的参数长度不能超过15个字符

mods=(Routing DBProxy Log Connect User Hall Room Game Fpf Daer GlDaer XyDaer HjDaer Gm)
prefix=${Prefix}

function check_alive() {
  for i in ${mods[@]};
  do
	local svrd=${prefix}${i}
	pgrep "${svrd}" > /dev/null && io_green "${svrd}Svrd alive ..." || io_red "${svrd}Svrd not alive ..."
  done
}

function io_green()
{
  is_empty "$1" && return 1
  io_color green "$@"
}

function io_red()
{
  is_empty "$1" && return 1
  io_color red "$@"
}

function is_empty() {
  [ -z "$1" ] && return 0 || return 1
}

function io_color() {
  local _var_color_red="\e[1m\e[31m"
  local _var_color_green="\e[1m\e[32m"
  local _var_color_yellow="\e[1m\e[33m"
  local _var_color_blue="\e[1m\e[34m"
  local _var_color_purple="\e[1m\e[35m"
  local _var_color_white="\e[1m\e[37m"
  local _var_color_end="\e[m"

  is_empty "$1" && return 1

  case "$1" in
    "red")    echo -ne ${_var_color_red}    ;;
    "green")  echo -ne ${_var_color_green}  ;;
    "yellow") echo -ne ${_var_color_yellow} ;;
    "blue")   echo -ne ${_var_color_blue}   ;;
    "purple") echo -ne ${_var_color_purple} ;;
    "white")  echo -ne ${_var_color_white}  ;;
    *)        echo "Unkown color: $1" && return 1 ;;
  esac

  #shift
  echo -e "$2"${_var_color_end}
}

check_alive;

