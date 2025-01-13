#!/bin/bash

cd $(dirname $0)

tap=true
test "$1" = "notap" && tap=false

redroot=/tmp/rpak-livetest
tmp=${redroot}.out

# cleaning
cleanup() {
    rm -rf "${redroot}" "${tmp}"
    for x in $(find /sys/fs/cgroup -type d | grep rpak-livetest | sort -r); do
        [[ -d "$x" ]] && rmdir "$x"
    done
}

print() {
    echo "$*" >&2
}

action() {
    print "$*"
    "$@"
}

perform_test() {
    local num="$1"

    print
    print "====== TEST ${num} ======="

    # program
    local prog="data/${num}-prog"

    # normal config
    local normal
    if test -f "data/${num}-normal.yaml"
    then
        normal="data/${num}-normal.yaml"
    else
        normal="data/0-normal.yaml"
    fi

    # admin config
    local admin flagadmin
    if test -f "data/${num}-admin.yaml"
    then
        admin="data/${num}-admin.yaml"
        flagadm="--admin"
    else
        admin="data/0-admin.yaml"
    fi

    # compile the C prog if existing
    if test -f "data/${num}-prog.c"
    then
        prog="data/${num}.prog"
        cc -static -o "${prog}" "data/${num}-prog.c"
    fi

    # reference and result filenames
    local refe="data/${num}-reference"
    local resu="data/${num}-result"

    # create the node
    local rednode="${redroot}/rnod${num}"
    action redconf create --alias "NODE-${num}" --config "${normal}" --config-adm "${admin}" "${rednode}"
    test $? -eq 0 || return

    # prepare the command
    local shell=$(awk '$1~/mount/&&$2~/busybox/{r=$2}END{print r}' "${normal}")
    local cmd="/testscript/${prog##*/}"
    if grep -q "${cmd}" "${normal}"
    then
        mkdir -p "$(dirname "${rednode}${cmd}")"
        cp "${prog}" "${rednode}${cmd}"
        chmod +x "${rednode}${cmd}"
    else
        cmd="$(cat "${prog}" | sed 's/#.*//;/^[ \n]*$/d;s/.*/ & /' | tr '\n' ';')"
    fi

    # execute the command in the node
    action env -i - PWD="$PWD" PATH="$PATH" INHERITED="this is an inherited value" EXTRA=extra redwrap $flagadm --redpath "${rednode}" -- ${shell} sh -c "${cmd}" |
        sed "s,$PWD,<<PWD>>,g" > "${resu}"
    test $? -eq 0 || return

    # compare result
    grep -v '\<IGNORE\>' "${resu}" |
    diff -N "${refe}" -
}

cleanup
$tap && echo "TAP version 14"
itap=0
for prog in $(ls data/*-prog* 2>/dev/null | sort -t/ -k2n)
do
    itap=$((itap + 1))
    num=$(echo -n "${prog}" | sed 's:.*/\([0-9]*\)-.*:\1:')
    title=$(head -n 1 "data/${num}-desc" 2>/dev/null|tr ' ' '-')
    perform_test "${num}" >& "${tmp}"
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
cleanup

