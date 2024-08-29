# bash completion for red-wrap                              -*- shell-script -*-

_redwrap()
{
    local cur prev words cword
    _init_completion -n = || return

    case $cur in
        --bwrap=*|--redpath=*)
            cur=${cur#*=}
            if [ -z "$cur" ]; then
                #Default path to use if cur is empty
                cur="."
            fi
            _filedir -d
            return
            ;;
    esac

    _expand || return

    COMPREPLY=( $( compgen -W '--redpath --bwrap' -S '=' -- "$cur" ) \
                $( compgen -W '--config --help --force'  -- "$cur" ))
}&&
complete -F _redwrap -o nospace red-wrap
