
pkg_check_modules(CHECK REQUIRED check)

include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)

# global test options
add_compile_definitions(TEMPLATES_DIR="${CMAKE_SOURCE_DIR}/etc/redpak/templates.d")
include_directories(${CMAKE_SOURCE_DIR}/red-wrap ${CMAKE_SOURCE_DIR}/red-conf)
link_libraries(libredconf libredwrap ${CHECK_LIBRARIES} pthread)

# absolute path of redmicrodnf
execute_process(COMMAND sh -c "readlink -f ${REDMICRODNF}" OUTPUT_VARIABLE REDMICRODNF_ABS OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT REDMICRODNF_ABS)
    message(STATUS "Not found REDMICRODNF = ${REDMICRODNF}, please install it or provide -DREDMICRODNF=<PATH>")
    add_compile_definitions(NO_REDMICRODNF_CMD=1)
else()
    message(STATUS "REDMICRODNF_CMD=${REDMICRODNF_ABS}")
    add_compile_definitions(REDMICRODNF_CMD="${REDMICRODNF_ABS}")
endif()

# checks of red-conf
add_executable(checks-conf checks-conf.c)
add_test(NAME checks-conf COMMAND checks-conf WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Testing)

# add preprocessor variable for tests

add_executable(checks checks.c)
add_test(NAME checks COMMAND checks WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Testing)

