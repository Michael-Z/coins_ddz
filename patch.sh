#! /bin/bash
#set -e
#set -x

function apply_patch() {
  for i in $@
  do
      svn patch $patch_file $i
  done
}

function usage() {
  if [ $1 -lt 2 ] 
  then
    echo "Usage: $0 [path_file] [patch_dirs]"
    echo "./patch.sh diff.patch svr/ddz_*"
  fi
}

usage $#;

patch_file=$1;
shift;

apply_patch "$@";
