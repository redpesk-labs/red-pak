# red-wrap completion  

_redwrap_options="
    --redpath=
    --bwrap=
    --config
    --help
    --force
"


_redwrap()
{

    IFS=$'\n'
    local options=$_redwrap_options
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    case $prev in
    --version)
       return 0
       ;;

    -h|--help)
        return 0
        ;;

    -c|--verbose)
        COMPREPLY=( $( compgen -W '0 1 2 3 4 5' -- "$cur" ) )
        return 0
        ;;

    --bwrap=*)
        _filedir -d
        return 0
        ;;

    --redpath=*)
        echo xxxxxxxx
        _filedir -d
        return 0
        ;;

    red-wrap)
        if test -z "$cur"
        then 
            red-wrap --help
            return 0
        fi    
        ;;
  
    *)
    esac

    case "$cur" in

    -*)
        #echo toto
        COMPREPLY=( $(compgen -S\= -W $options -- ${cur}) )
        return 0
        ;;

    esac    

    return 0
}

complete -F _redwrap red-wrap
