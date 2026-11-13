#!/bin/bash

_metacg_completion() {

    # _comp_initialize is a helper of bash-completions that populates variables
    # cur - The word currently completed
    # prev - The word immediatly before the current word
    # words - Array containing command-line words
    # cword - Index of the current word in the words array
    local cur prev words cword
    local COMP_WORDBREAKS="${COMP_WORDBREAKS//=/}"

    # bash-completion >2.12 provides _comp_initialize, older versions only
    # provide the equivalent (now deprecated) _init_completion
    if declare -F _comp_initialize >/dev/null; then
        _comp_initialize || return
    else
        _init_completion || return
    fi


    # Retrieve the command-line words while treating '=' as part of the current
    # word rather than as a word separator. This is necessary to support
    # completion of options using the --option=value syntax.
    # Prefer the newer _comp_get_words helper and fall back to the older
    # _get_comp_words_by_ref helper for compatibility with older versions.
    if declare -F _comp_get_words >/dev/null; then
        _comp_get_words -n = cur prev words cword || return
    elif declare -F _get_comp_words_by_ref >/dev/null; then
        _get_comp_words_by_ref -n = cur prev words cword || return
    fi

    # Determine executable
    local cmd="${words[0]}"

    # Ask the executable for possible completions (inspired by g++ completion script)
    # The executable handles the --completion argument and print matching candidates,
    # one per line to stdout
    COMPREPLY=( $( "$cmd" --completion="$cur" 2>/dev/null ) )


    # Prevent Bash from appending a space after options ending in '='
    if [[ "${COMPREPLY[0]}" == *= ]]; then
        compopt -o nospace
    fi
}


# Register the completion function
# Bash calls _metacg_completion whenever completion is requested
complete -F _metacg_completion cgcollector2
complete -F _metacg_completion cgdiff
complete -F _metacg_completion cgformat
complete -F _metacg_completion cgtodot
