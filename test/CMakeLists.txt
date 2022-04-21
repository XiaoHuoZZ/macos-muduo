
add_executable(test main.cc)

target_link_libraries(test lib_net)
target_link_libraries(test lib_base)