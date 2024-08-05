PROJECT_TARGET_ADD(checks)
    find_package(Check REQUIRED)

    include(CheckCSourceCompiles)
    include(CheckCSourceRuns)
    include(CheckFunctionExists)
    include(CheckIncludeFile)
    include(CheckIncludeFiles)
    include(CheckLibraryExists)
    include(CheckSymbolExists)
    include(CheckTypeSize)

    # absolute path of redmicrodnf
    execute_process(COMMAND sh -c "readlink -f ${REDMICRODNF}" OUTPUT_VARIABLE REDMICRODNF_ABS OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT REDMICRODNF_ABS)
        message(FATAL_ERROR "Not found REDMICRODNF = ${REDMICRODNF}, please install it or provide -DREDMICRODNF=<PATH>")
    endif()

    add_executable(${TARGET_NAME} checks.c)
    message(STATUS "REDMICRODNF_CMD=${REDMICRODNF_ABS}")

    # add preprocessor variable for tests
    target_compile_options(${TARGET_NAME} PUBLIC -DREDMICRODNF_CMD="${REDMICRODNF_ABS}")
    target_compile_options(${TARGET_NAME} PUBLIC -DTEMPLATES_DIR="${CMAKE_SOURCE_DIR}/etc/redpak/templates.d")

    target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../red-wrap ${CMAKE_CURRENT_SOURCE_DIR}/../red-conf)
    target_link_libraries(${TARGET_NAME} libredconf libredwrap ${CHECK_LIBRARIES} pthread)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Testing)


