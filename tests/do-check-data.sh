#!/bin/bash

rednode=/tmp/redpak-check-data
for prog in $(ls data/*-prog 2>/dev/null | sort -t/ -k2n)
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
    bbox=$(awk -vr=busybox '$1~/mount/&&$2~/busybox/{r=$2}END{print r}' "${normal}")

    echo
    echo "====== TEST ${num} ======="

    # create the node
    rm -rf "${rednode}"
    echo "redconf create --alias NODE-${num} --config ${normal} --config-adm ${admin} ${rednode}"
    redconf create --alias "NODE-${num}" --config "${normal}" --config-adm "${admin}" "${rednode}"

    cmd="/testscript/${prog##*/}"
    if grep -q "${cmd}" "${normal}"
    then
        mkdir -p "$(dirname "${rednode}${cmd}")"
        cp "${prog}" "${rednode}${cmd}"
        chmod +x "${rednode}${cmd}"
    else
        cmd="$(cat "${prog}" | sed 's/#.*//;/^[ \n]*$/d;s/.*/ & /' | tr '\n' ';')"
    fi

    echo "redwrap --redpath ${rednode} -- ${bbox} sh -c ${cmd}"
    redwrap --redpath "${rednode}" -- "${bbox}" sh -c "${cmd}" > "${resu}"
    diff "${refe}" "${resu}"
done
echo
rm -rf "${rednode}"
