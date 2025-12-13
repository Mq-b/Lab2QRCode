cmake_minimum_required(VERSION 3.10)

project(Lab2QRCode)

include(scripts/cmake/check-format.cmake) # 检查格式，如果格式错误则失败
include(scripts/cmake/check-format-prebuild.cmake) # 在构建前检查格式，但格式错误仍然能继续构建
include(scripts/cmake/format-all.cmake) # 格式化所有代码
include(scripts/cmake/version-info.cmake) # 生成版本信息

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

include_directories(include)

set(CMAKE_PREFIX_PATH $ENV{VCPKG_LIB} ${CMAKE_PREFIX_PATH})
#set(CMAKE_PREFIX_PATH  $ENV{QT5_15_STATIC} ${CMAKE_PREFIX_PATH})

if(MSVC)
  add_compile_options(/EHsc /utf-8 /bigobj)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Concurrent Multimedia MultimediaWidgets)
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

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${VERSION_INFO_FILE} "logo.rc")
add_dependencies(${PROJECT_NAME} CheckFormatPrebuild GenVersionInfo)

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
