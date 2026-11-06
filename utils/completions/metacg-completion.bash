#!/bin/bash

_metacg_completion() {

    # _comp_initialize is a helper of bash-completions that populates variables
    # cur - The word currently completed
    # prev - The word immediatly before the current word
    # words - Array containing command-line words
    # cword - Index of the current word in the words array
    local cur prev words cword

    _comp_initialize

    # Determine executable
    local cmd="${words[0]}"

    # Ask the executable for possible completions (inspired by g++ completion script)
    # The executable handles the --completion argument and print matching candidates,
    # one per line to stdout

    # FIXME: This currently assumes cmd can be found through PATH and its name is words[0]

    COMPREPLY=( $( "$cmd" --completion="$cur") )
}


# Register the completion function
# Bash calls _metacg_completion whenever completion is requested

complete -F _metacg_completion cgcollector2
complete -F _metacg_completion cgdiff
