#!/bin/bash

rednode=/tmp/redpak-check-data
for prog in data/*-prog
do
    num=$(echo -n "${prog}" | sed 's:.*/\([0-9]*\)-.*:\1:')
    if test -f "data/${num}-normal.yaml"
    then
        normal="data/${num}-normal.yaml"
    else
        normal="data/0-normal.yaml"
    fi
    if test -f "data/${num}-admin.yaml"
    then
        admin="data/${num}-admin.yaml"
    else
        admin="data/0-admin.yaml"
    fi
    refe="data/${num}-reference"
    resu="data/${num}-result"
    cmd="$(cat "${prog}" | sed 's/#.*//;/^[ \n]*$/d;s/.*/ & /' | tr '\n' ';')"
    bbox=$(awk -vr=busybox '$1~/mount/&&$2~/busybox/{r=$2}END{print r}' "${normal}")

    echo
    echo "====== TEST ${num} ======="
    rm -rf "${rednode}"
    echo "redconf create --alias NODE-${num} --config ${normal} --config-adm ${admin} ${rednode}"
    redconf create --alias "NODE-${num}" --config "${normal}" --config-adm "${admin}" "${rednode}"
    echo "redwrap --redpath ${rednode} -- ${bbox} sh -c ${cmd}"
    redwrap --redpath "${rednode}" -- "${bbox}" sh -c "${cmd}" > "${resu}"
    diff "${refe}" "${resu}"
done
echo
rm -rf "${rednode}"
