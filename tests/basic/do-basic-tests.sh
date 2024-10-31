#!/bin/bash

# 
tap=true
case "$1" in
  notap|--notap|-v) tap=false; shift;;
esac

btpref=BT-
tmp=/tmp/redpak-check-data-output

rm -rf rednodes configs
mkdir -p rednodes configs results
$(dirname $0)/gen-confs.sh "configs/"

#---------------------------------------
# creation of a rednode
#---------------------------------------
makenode() {
    local name="$1" node="$2" normal="$3" admin="$4"
    echo "redconf create --alias ${name} --config ${normal} --config-adm ${admin} ${node}"
    if ! test -d "${PWD}/rednodes/${node}"
    then
        mkdir -p "${PWD}/rednodes/${node}"
        redconf create --alias "${name}" --config "${PWD}/configs/${normal}" --config-adm "${PWD}/configs/${admin}" "${PWD}/rednodes/${node}"
    fi
}

#---------------------------------------
# show the bwrap calling arguments
#---------------------------------------
showbwrap() {
    local opts=
    while [[ $# -gt 1 ]]; do opts="$opts $1"; shift; done
    local node="$1"
    echo "redwrap $opts --dump-only --redpath ${node}"
    redwrap $opts --dump-only --redpath "${PWD}/rednodes/${node}"
}

#---------------------------------------
# mix the given pair of config in many ways and print results
#---------------------------------------
testu() {
    # gets parameters
    local conf1="${1}"
    local conf2="${2}"

    # compute of path of rednodes
    local node="node/${conf1}/${conf2}"
    local rootnn="rootnn/${conf1}"
    local rootna="rootna/${conf1}"
    local rootan="rootan/${conf1}"
    local rootaa="rootaa/${conf1}"
    local subnn="rootnn/${conf1}/${conf2}"
    local subna="rootna/${conf1}/${conf2}"
    local suban="rootan/${conf1}/${conf2}"
    local subaa="rootaa/${conf1}/${conf2}"

    echo
    echo "==================================="
    echo "== CONF1 $conf1"
    echo "==================================="
    cat "${PWD}/configs/${conf1}"
    echo

    echo "==================================="
    echo "== CONF2 $conf2"
    echo "==================================="
    cat "${PWD}/configs/${conf2}"
    echo

    echo "==================================="
    makenode "NODE" "${node}" "${conf1}" "${conf2}" 
    showbwrap "${node}"
    showbwrap --admin "${node}"
    echo

    echo "==================================="
    makenode "ROOT" "${rootnn}" "${conf1}" NULL
    makenode "SUBNODE" "${subnn}" "${conf2}" NULL
    showbwrap "${subnn}"
    showbwrap --admin "${subnn}"
    echo

    echo "==================================="
    makenode "ROOT" "${rootna}" "${conf1}" NULL
    makenode "SUBNODE" "${subna}" NULL "${conf2}"
    showbwrap "${subna}"
    showbwrap --admin "${subna}"
    echo

    echo "==================================="
    makenode "ROOT" "${rootan}" NULL "${conf1}"
    makenode "SUBNODE" "${suban}" "${conf2}" NULL
    showbwrap "${suban}"
    showbwrap --admin "${suban}"
    echo

    echo "==================================="
    makenode "ROOT" "${rootaa}" NULL "${conf1}"
    makenode "SUBNODE" "${subaa}" NULL "${conf2}"
    showbwrap "${subaa}"
    showbwrap --admin "${subaa}"
    echo
}

filter='s,'"$PWD"',,g;/^\[[A-Z]*]: /{s/\[[^][]*] *$//;s/[ \t]*$//}'

xtap() {
    local conf1="${1}"
    local conf2="${2}"
    local id="${conf1}:${conf2}"
    local resu="results/${id}"
    local ref="references/${id}"
    itap=$((itap + 1))
    testu "${conf1}" "${conf2}" |& sed "$filter" > "${resu}"
    diff "$ref" "${resu}" >& "${tmp}"
    sts=$?
    if $tap; then
        if test $sts -gt 0; then
            echo "not ok $itap - ${id}"
            sed 's:^:   :' "${tmp}"
        else
            echo "ok $itap - ${id}"
        fi
    else
        cat "${tmp}"
    fi
}

pnul() {
    local x
    for x in $@
    do
        xtap "$x" "NULL"
    done
}

punic() {
    local x
    for x in $@
    do
        xtap "$x" "$x"
    done
}

pmix() {
    local x y
    for x in $@
    do
        for y in $@
        do
            xtap "$x" "$y"
        done
    done
}

#---------------------------------------
# first tap
$tap && echo "TAP version 14"
itap=0

pnul \
  config-cachedir \
  config-capa-cap_chown-0 \
  config-capa-cap_chown-1 \
  config-capa-cap_setuid-0 \
  config-capa-cap_setuid-1 \
  config-cgrouproot \
  config-chdir \
  config-die-with-parent-disabled \
  config-die-with-parent-enabled \
  config-die-with-parent-unset \
  config-gpgcheck-false \
  config-gpgcheck-true \
  config-hostname \
  config-ldpath \
  config-new-session-disabled \
  config-new-session-enabled \
  config-new-session-unset \
  config-path \
  config-persistdir \
  config-rpmdir \
  config-share_all-disabled \
  config-share_all-enabled \
  config-share_all-unset \
  config-share_cgroup-disabled \
  config-share_cgroup-enabled \
  config-share_cgroup-unset \
  config-share_ipc-disabled \
  config-share_ipc-enabled \
  config-share_ipc-unset \
  config-share_net-disabled \
  config-share_net-enabled \
  config-share_net-unset \
  config-share_pid-disabled \
  config-share_pid-enabled \
  config-share_pid-unset \
  config-share_time-disabled \
  config-share_time-enabled \
  config-share_time-unset \
  config-share_user-disabled \
  config-share_user-enabled \
  config-share_user-unset \
  config-umask-027 \
  config-umask-077 \
  config-unsafe-false \
  config-unsafe-true \
  config-verbose-0 \
  config-verbose-15 \
  environ-default \
  environ-execfd \
  environ-remove \
  environ-static \
  environ-unset \
  export-anonymous-A \
  export-devfs-A \
  export-execfd-A \
  export-internal-A \
  export-lock-A \
  export-mqueue-A \
  export-private-A \
  export-privatefile-A \
  export-privaterestricted-A \
  export-procfs-A \
  export-public-A \
  export-publicfile-A \
  export-restricted-A \
  export-restrictedfile-A \
  export-symlink-A \
  export-tmpfs-A











punic              \
  config-cachedir   \
  config-cgrouproot \
  config-chdir      \
  config-hostname   \
  config-ldpath     \
  config-path       \
  config-persistdir \
  config-rpmdir

pmix \
  config-capa-cap_chown-0  \
  config-capa-cap_chown-1  \
  config-capa-cap_setuid-0  \
  config-capa-cap_setuid-1

pmix \
  config-die-with-parent-disabled \
  config-die-with-parent-enabled \
  config-die-with-parent-unset

pmix \
  config-gpgcheck-false \
  config-gpgcheck-true

pmix \
  config-new-session-disabled \
  config-new-session-enabled

pmix \
  config-share_all-disabled \
  config-share_all-enabled \
  config-share_ipc-disabled \
  config-share_ipc-enabled

pmix \
  config-share_ipc-disabled \
  config-share_ipc-enabled \
  config-share_net-disabled \
  config-share_net-enabled

pmix \
  config-umask-027 \
  config-umask-077

pmix \
  environ-default \
  environ-execfd \
  environ-remove \
  environ-static

pmix \
  environ-remove \
  environ-unset

pmix \
  export-anonymous-A  \
  export-anonymous-B

pmix \
  export-devfs-A  \
  export-devfs-B

pmix \
  export-execfd-A  \
  export-execfd-B

pmix \
  export-internal-A  \
  export-internal-B

pmix \
  export-private-A  \
  export-private-B

pmix \
  export-privatefile-A  \
  export-privatefile-B

pmix \
  export-privaterestricted-A  \
  export-privaterestricted-B

pmix \
  export-public-A  \
  export-public-B

pmix \
  export-publicfile-A  \
  export-publicfile-B

pmix \
  export-restricted-A  \
  export-restricted-B

pmix \
  export-restrictedfile-A  \
  export-restrictedfile-B

pmix \
  export-lock-A  \
  export-lock-B

pmix \
  export-mqueue-A  \
  export-mqueue-B

pmix \
  export-procfs-A  \
  export-procfs-B

pmix \
  export-symlink-A  \
  export-symlink-B

pmix \
  export-tmpfs-A  \
  export-tmpfs-B


pmix \
  export-internal-A  \
  export-private-A  \
  export-privatefile-A  \
  export-privaterestricted-A  \
  export-public-A  \
  export-publicfile-A  \
  export-restricted-A  \
  export-restrictedfile-A


$tap && echo "1..${itap}"

echo

