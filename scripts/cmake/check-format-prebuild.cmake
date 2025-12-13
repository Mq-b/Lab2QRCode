if(UNIX)
  add_custom_target(
    CheckFormatPrebuild
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/build/check-format-prebuild.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
else()
  add_custom_target(
    CheckFormatPrebuild
    COMMAND pwsh -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/scripts/build/check-format-prebuild.ps1"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
