if(NOT WITH_MICRODNF)
    return()
endif()

find_package(Threads)
pkg_check_modules(LIBFMT REQUIRED fmt)
pkg_check_modules(SMARTCOLS REQUIRED smartcols)

add_definitions(-DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# use any sources found under the current directory
file(GLOB_RECURSE MICRODNF_SOURCES *.cpp)

add_executable(microdnf ${MICRODNF_SOURCES})

target_include_directories(microdnf PUBLIC ../red-lib ../../red-conf)
target_link_directories(microdnf PUBLIC ${CMAKE_BINARY_DIR}/red-microdnf/red-lib)
target_link_libraries(microdnf PUBLIC dnf dnf-cli ${LIBFMT_LIBRARIES} ${SMARTCOLS_LIBRARIES} stdc++ stdc++fs Threads::Threads libredlib.so)
install(TARGETS microdnf RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
