#!/bin/bash

tap=true
test "$1" = "notap" && tap=false

redroot=/tmp/rpak-conftest
tmp=${redroot}.out

# cleaning

filter='s,\[/[^]i:]*:[0-9]*][ \t]*$,,;s,[ \t]*$,,'

do_test() {
    local input="$1" output="$2" reference="$3"
    rm -rf "${redroot}"
    mkdir -p "${redroot}/etc"
    cp "${input}" "${redroot}/etc/redpak.yaml"
    cat << EOC > "${redroot}/.rednode.yaml"
realpath: ${redroot}
info: created by do-conf-tests.sh the Wed Jan  1 07:00:00 UTC 2020
timestamp: 1577862000000
state: Enable
EOC
    echo "redconf --yaml config ${redroot} > ${output}"
    redconf --yaml config "${redroot}" |&
      sed "${filter}" >& "${output}"
    grep -v '\<IGNORE\>' "${output}" |
      diff -N "${reference}" -
}

rm -rf "${redroot}" "${tmp}"
$tap && echo "TAP version 14"
itap=0
for num in $(ls data/*-input 2>/dev/null | sed 's:data/::;s:-input::' | sort -n)
do
    itap=$((itap + 1))
    input="data/${num}-input"
    output="data/${num}-output"
    reference="data/${num}-reference"
    title="test-conf-${num}"
    do_test "${input}" "${output}" "${reference}" >& "${tmp}"
    sts=$?
    if $tap; then
        if test $sts -gt 0; then
            echo "not ok $itap - ${title}"
            sed 's:^:   :' "${tmp}"
        else
            echo "ok $itap - ${title}"
        fi
    else
        cat "${tmp}"
    fi
done
$tap && echo "1..${itap}"
echo
