function(set_policies)
  # Choose modern CMake behavior when extracting archives
  if(POLICY CMP0135)
    cmake_policy(
      SET
      CMP0135
      NEW
    )
  endif()
endfunction()
