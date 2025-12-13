if(UNIX)
  add_custom_target(
    FormatAll
    COMMAND ${CMAKE_COMMAND} -E echo "Formatting code..."
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/build/format-all.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "bash ${CMAKE_SOURCE_DIR}/scripts/build/format-all.sh")
else()
  add_custom_target(
    FormatAll
    COMMAND ${CMAKE_COMMAND} -E echo "Formatting code..."
    COMMAND pwsh -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/scripts/build/format-all.ps1"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "powershell -ExecutionPolicy Bypass -File \"${CMAKE_SOURCE_DIR}/scripts/build/format-all.ps1\"")
endif()
