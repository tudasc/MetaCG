function(
  find_llvm_progs
  target
  names
)
  cmake_parse_arguments(
    ARG
    "ABORT_IF_MISSING;SHOW_VAR"
    "DEFAULT_EXE"
    "HINTS"
    ${ARGN}
  )
  set(TARGET_TMP ${target})

  find_program(
    ${target}
    NAMES ${names}
    PATHS ${LLVM_TOOLS_BINARY_DIR}
    NO_DEFAULT_PATH
  )
  if(NOT ${target})
    find_program(
      ${target}
      NAMES ${names}
      HINTS ${ARG_HINTS}
    )
  endif()

  if(NOT ${target})
    set(target_missing_message "")
    if(ARG_DEFAULT_EXE)
      unset(${target} CACHE)
      set(${target}
          ${ARG_DEFAULT_EXE}
          CACHE STRING "Default value for ${TARGET_TMP}."
      )
      set(target_missing_message "Using default: ${ARG_DEFAULT_EXE}")
    endif()

    set(message_status STATUS)
    if(ARG_ABORT_IF_MISSING AND NOT ARG_DEFAULT_EXE)
      set(message_status SEND_ERROR)
    endif()
    message(
      ${message_status}
      "Did not find LLVM program "
      "${names}"
      " in ${LLVM_TOOLS_BINARY_DIR}, in system path or hints "
      "\"${ARG_HINTS}\""
      ". "
      ${target_missing_message}
    )
  endif()

  if(ARG_SHOW_VAR)
    mark_as_advanced(CLEAR ${target})
  else()
    mark_as_advanced(${target})
  endif()
endfunction()
