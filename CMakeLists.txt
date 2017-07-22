cmake_minimum_required (VERSION 2.6)
project (Jack)
# The version number.

find_package(PkgConfig REQUIRED)
pkg_search_module(JACK REQUIRED jack)

add_executable(jack_test jack_test.cc)
# add the executable
target_link_libraries(jack_test ${JACK_LIBRARIES})
target_include_directories(jack_test PUBLIC ${JACK_INCLUDE_DIRS})
target_compile_options(jack_test PUBLIC ${JACK_CFLAGS_OTHER})
