
add_executable(test main.cc)

target_link_libraries(test net)
target_link_libraries(test base)
target_link_libraries(test pthread)