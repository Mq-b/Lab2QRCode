set(VERSION_INFO_FILE "${CMAKE_SOURCE_DIR}/src/version_info/version.cpp")
add_custom_target(
  GenVersionInfo
  COMMAND ${CMAKE_COMMAND} -E echo "Generating version info..."
  COMMAND pwsh -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/scripts/build/version-info.ps1"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/src/version_info"
  COMMENT "powershell -ExecutionPolicy Bypass -File \"${CMAKE_SOURCE_DIR}/scripts/build/version-info.ps1\""
  BYPRODUCTS ${VERSION_INFO_FILE})
