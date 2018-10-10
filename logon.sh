#! /bin/bash
set -e
set -x
sig=36
./p.sh | awk '{print $2}' | xargs -i kill -$sig {}
echo "============= set log on ... =============="
