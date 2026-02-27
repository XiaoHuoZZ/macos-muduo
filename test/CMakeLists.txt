
add_executable(server_test server_test.cc)
add_executable(client_test client_test.cc)
set(ALL_TEST server_test client_test)

# Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(timer_test timer.cc)
    
    # 将 timer_test 追加到列表中
    list(APPEND ALL_TEST timer_test)
endif()

foreach(test ${ALL_TEST})
    target_link_libraries(${test} net)
    target_link_libraries(${test} base)
    target_link_libraries(${test} pthread)
endforeach()