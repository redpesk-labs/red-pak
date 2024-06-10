#!/bin/sh
export PATH=~/redpesk/iotbzh/iotbzh-engineering-process/scripts:$PATH
CDPATH=
cd $(dirname $0)
mkdir -p site
mk-mat.sh --html > site/TRACABILITY.html
mk-site.sh $* -o site QA -T "redpesk-core/sec-lsm-manager main index" -S index-order
