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

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${VERSION_CPP} "logo.rc")
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

# ========================================
# 翻译文件自动更新、自动生成、自动拷贝到可执行生成
# 设置ts文件目录
set(TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/app_en_US.ts
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/app_zh_CN.ts
)

# 每次构建都自动更新 .ts 文件
add_custom_target(update_translations ALL
    COMMAND ${Qt5_LUPDATE_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            -ts ${TS_FILES}
            -no-obsolete
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Updating .ts files"
)

# 生成 .qm 文件
qt5_add_translation(QM_FILES ${TS_FILES})
# QM_FILES添加进sources，否则不会生成qm文件
target_sources(${PROJECT_NAME} PRIVATE ${QM_FILES})

# 拷贝 .qm 文件到 bin/translations 目录下
foreach(QM_FILE IN LISTS QM_FILES)
    get_filename_component(QM_FILENAME ${QM_FILE} NAME)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QM_FILE}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/translations/${QM_FILENAME}"
        COMMENT "Copying ${QM_FILENAME} to translations folder"
    )
endforeach()
# ========================================