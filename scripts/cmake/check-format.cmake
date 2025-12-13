if(UNIX)
  add_custom_target(
    CheckFormat
    COMMAND ${CMAKE_COMMAND} -E echo "Checking code format..."
    COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/build/check-format.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "bash ${CMAKE_SOURCE_DIR}/scripts/build/check-format.sh")
else()
  add_custom_target(
    CheckFormat
    COMMAND ${CMAKE_COMMAND} -E echo "Checking code format..."
    COMMAND pwsh -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/scripts/build/check-format.ps1"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "powershell -ExecutionPolicy Bypass -File \"${CMAKE_SOURCE_DIR}/scripts/build/check-format.ps1\"")
endif()
