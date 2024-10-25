#!/bin/bash

allcapabilities="
    cap_chown
    cap_dac_override
    cap_dac_read_search
    cap_fowner
    cap_fsetid
    cap_kill
    cap_setgid
    cap_setuid
    cap_setpcap
    cap_linux_immutable
    cap_net_bind_service
    cap_net_broadcast
    cap_net_admin
    cap_net_raw
    cap_ipc_lock
    cap_ipc_owner
    cap_sys_module
    cap_sys_rawio
    cap_sys_chroot
    cap_sys_ptrace
    cap_sys_pacct
    cap_sys_admin
    cap_sys_boot
    cap_sys_nice
    cap_sys_resource
    cap_sys_time
    cap_sys_tty_config
    cap_mknod
    cap_lease
    cap_audit_write
    cap_audit_control
    cap_setfcap
    cap_mac_override
    cap_mac_admin
    cap_syslog
    cap_wake_alarm
    cap_block_suspend
    cap_audit_read
    cap_perfmon
    cap_bpf
    cap_checkpoint_restore"

listcapabilities="cap_chown cap_setuid"

#---------------------------------------------------------

prefix="$1"

tcase() {
    if [[ -z "$prefix" ]]
    then
        echo
        echo "TCASE $*"
    else
        exec > "${prefix}$*"
    fi
    echo "headers:"
    echo "  alias: alias"
    echo "  name: name"
    echo "  info: info"
}

#---------------------------------------------------------

tconfig() {
    local id="$1" key="$2" value="$3"
    tcase "config-$id"
    echo "config:"
    echo "   $key: $value"
}

mkconf1() {
    for key in share_all share_user share_cgroup share_net share_pid share_ipc share_time die-with-parent new-session
    do
        for val in disabled enabled unset
        do
            tconfig "$key-$val" "$key" "$val"
        done
    done
}

mkconf2() {
    for key in rpmdir persistdir cachedir path ldpath
    do
        tconfig $key $key '$NODE_PATH:$NODE_ALIAS:$LEAF_ALIAS'
    done
    tconfig verbose-0 verbose 0
    tconfig verbose-15 verbose 15
    tconfig umask-027 verbose 027
    tconfig umask-077 verbose 077

    tconfig hostname hostname '$LEAF_ALIAS'
    tconfig chdir chdir '/home/$LEAF_ALIAS'

    tconfig cgrouproot cgrouproot '/sys/fs/cgroup/user.slice/user-$UID.slice/user@$UID.service'
}

mkconf3() {
    for key in gpgcheck unsafe
    do
        for val in true false
        do
            tconfig "$key-$val" "$key" "$val"
        done
    done
}

tconfcapa() {
    local id="$1" capa="$2" value="$3"
    tcase "config-capa-$id"
    echo "config:"
    echo "   capabilities:"
    echo "     - cap: $capa"
    echo "       add: $value"
}

mkconf4() {
    for key in $listcapabilities
    do
        for val in 0 1
        do
            tconfcapa "$key-$val" "$key" "$val"
        done
    done
}

#---------------------------------------------------------

tenviron() {
    local id="$1" key="$2" mode="$3" value="$4"
    tcase "environ-$id"
    echo "environ:"
    echo "  - key: $key"
    [[ -z "$mode"  ]] || echo "    mode: $mode"
    [[ -z "$value" ]] || echo "    value: $value"
}

mkenv() {
    tenviron unset   VAR ''      "'\$NODE_ALIAS(\$LEAF_ALIAS)> '"
    tenviron default VAR default "'\$NODE_ALIAS(\$LEAF_ALIAS)> '"
    tenviron static  VAR static  "'\$NODE_ALIAS(\$LEAF_ALIAS)> '"
    tenviron execfd  VAR execfd  "'echo HELLO'"
    tenviron remove  VAR remove
}

#---------------------------------------------------------

texport() {
    local id="$1" mode="$2" mount="$3" path="$4"
    tcase "export-$id"
    echo "exports:"
    echo "  - mode: $mode"
    echo "    mount: $mount"
    [[ -z "$path" ]] || echo "    path: $path"
}

mkexp1() {
    for mode in restricted public private privaterestricted restrictedfile publicfile privatefile symlink internal
    do
        for x in A B
        do
            texport "$mode-$x" "$mode" "/$x" "/somewhere"
        done
    done
    for x in A B
    do
        texport "execfd-$x" execfd "/$x" "'echo /somewhere/$x'"
    done
}

mkexp2() {
    for mode in anonymous tmpfs procfs devfs mqueue lock
    do
        for x in A B
        do
            texport "$mode-$x" "$mode" "/$x"
        done
    done
}

#---------------------------------------------------------

tcase NULL
mkconf1
mkconf2
mkconf3
mkconf4
mkenv
mkexp1
mkexp2
