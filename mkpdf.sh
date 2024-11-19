#!/bin/sh
export PATH=~/redpesk/iotbzh/iotbzh-engineering-process/scripts:$PATH
CDPATH=
$(dirname $0)/mksite.sh
cd $(dirname $0)
mkdir -p pdfs
mk-pdfs.sh
