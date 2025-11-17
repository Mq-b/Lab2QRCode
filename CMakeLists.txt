cmake_minimum_required(VERSION 3.10)

project(Lab2QRCode)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin)

include_directories(include)

set(CMAKE_PREFIX_PATH $ENV{VCPKG_LIB} ${CMAKE_PREFIX_PATH})
#set(CMAKE_PREFIX_PATH  $ENV{QT5_15_STATIC} ${CMAKE_PREFIX_PATH})

if(MSVC)
    add_compile_options(/EHsc /utf-8 /bigobj)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Threads REQUIRED)
find_package(ZXing REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS
    headers
    random
)
find_package(spdlog CONFIG REQUIRED)

get_target_property(ZXING_INCLUDES ZXing::ZXing INTERFACE_INCLUDE_DIRECTORIES)

message(STATUS "ZXing include path = ${ZXING_INCLUDES}")
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} "logo.rc")

target_include_directories(${PROJECT_NAME} PRIVATE ${ZXING_INCLUDES})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt5::Core
    Qt5::Widgets
    ZXing::ZXing
    ${OpenCV_LIBS}
    Boost::headers
    Boost::random
    spdlog::spdlog
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/setting"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/setting"
)