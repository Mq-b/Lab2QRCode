cmake_minimum_required(VERSION 3.10)

project(Lab2QRCode)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

include_directories(include)

set(CMAKE_PREFIX_PATH $ENV{VCPKG_LIB} ${CMAKE_PREFIX_PATH})
#set(CMAKE_PREFIX_PATH  $ENV{QT5_15_STATIC} ${CMAKE_PREFIX_PATH})

set(VERSION_CPP "${CMAKE_SOURCE_DIR}/src/version_info/version.cpp")

add_custom_target(
  RunPowerShellScript ALL
  COMMAND ${CMAKE_COMMAND} -E echo "Running PowerShell script..."
  COMMAND pwsh -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/scripts/build/version_info.ps1
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/version_info
  COMMENT "powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/version_info.ps1"
  BYPRODUCTS ${VERSION_CPP})

if(MSVC)
  add_compile_options(/EHsc /utf-8 /bigobj)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Concurrent Multimedia MultimediaWidgets LinguistTools)
find_package(Threads REQUIRED)
find_package(ZXing REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS headers random)
find_package(spdlog CONFIG REQUIRED)

find_path(XLSXWRITER_INCLUDE_DIR xlsxwriter.h)
find_library(XLSXWRITER_LIBRARY NAMES xlsxwriter)
include_directories(${XLSXWRITER_INCLUDE_DIR})
if(NOT XLSXWRITER_INCLUDE_DIR OR NOT XLSXWRITER_LIBRARY)
  message(FATAL_ERROR "Could not find xlsxwriter library or include directory")
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

# 获取 lupdate 和 lrelease 的实际路径
get_target_property(QT_LUPDATE_EXECUTABLE Qt5::lupdate LOCATION)
get_target_property(QT_LRELEASE_EXECUTABLE Qt5::lrelease LOCATION)

# 翻译文件生成脚本路径
set(TRANSLATION_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/configure/generate_translations.ps1)

# 在 configure 阶段执行generate_translations.ps1脚本
execute_process(
    COMMAND powershell -ExecutionPolicy Bypass -File ${TRANSLATION_SCRIPT}
            "${QT_LUPDATE_EXECUTABLE}"
            "${QT_LRELEASE_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE SCRIPT_RESULT
    ECHO_OUTPUT_VARIABLE
)

if(NOT SCRIPT_RESULT EQUAL 0)
    message(FATAL_ERROR "Translation script failed!")
endif()


add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${VERSION_CPP} "logo.rc" resources/i18n.qrc)
add_dependencies(${PROJECT_NAME} RunPowerShellScript)

target_link_libraries(${PROJECT_NAME} PRIVATE 
  Qt5::Core
  Qt5::Widgets
  Qt5::Concurrent
  Qt5::Multimedia
  Qt5::MultimediaWidgets
  ZXing::ZXing
  ${OpenCV_LIBS}
  Boost::headers
  Boost::random
  spdlog::spdlog_header_only
  ${XLSXWRITER_LIBRARY}
)

if(MSVC)
  target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt)
endif()

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/setting" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/setting"
)
