# Build CGPatch: cgpatch-pass, cgpatch-runtime, wrappers, tests
option(
  METACG_BUILD_CGPATCH
  "On or off"
  OFF
)
if(METACG_BUILD_CGPATCH)
  if(${LLVM_PACKAGE_VERSION}
     GREATER
     14
  )
    add_subdirectory(cgpatch)
  else()
    message(WARNING "Skipping CGPatch: requires LLVM version 15 or higher")
  endif()
endif()

add_subdirectory(cgmerge2)
add_subdirectory(cgconvert)
add_subdirectory(cgformat)
