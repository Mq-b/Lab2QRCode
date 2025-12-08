find_package(Git)

set(GIT_HASH "")
set(GIT_TAG "")
set(GIT_BRANCH "master")
set(GIT_COMMIT_TIME "")

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH_DETECTED
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_BRANCH_DETECTED)
        set(GIT_BRANCH ${GIT_BRANCH_DETECTED})
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd "--date=format:%Y-%m-%d %H:%M:%S"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_TIME
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

# generate the building time
set(BUILD_TIME "")

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    set(_PY_SCRIPT "")
    string(APPEND _PY_SCRIPT
        "from datetime import datetime, timezone, timedelta\n"
        "tz = timezone(timedelta(hours=8))\n"
        "now_cn = datetime.now(tz)\n"
        "print(now_cn.strftime('%Y-%m-%d %H:%M:%S'), end='')\n"
    )

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "${_PY_SCRIPT}"
        OUTPUT_VARIABLE BUILD_TIME
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(NOT BUILD_TIME)
    string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")
endif()

set(VERSION_CONTENT "")
string(APPEND VERSION_CONTENT
    "#include \"version.h\"\n\n"
    "namespace version{\n"
    "    constexpr std::string_view git_hash = \"${GIT_HASH}\";\n"
    "    constexpr std::string_view git_tag = \"${GIT_TAG}\";\n"
    "    constexpr std::string_view git_branch = \"${GIT_BRANCH}\";\n"
    "    constexpr std::string_view git_commit_time = \"${GIT_COMMIT_TIME}\";\n"
    "    constexpr std::string_view build_time = \"${BUILD_TIME}\";\n"
    "};\n"
)

file(WRITE "${OUTPUT_FILE}" "${VERSION_CONTENT}")

12