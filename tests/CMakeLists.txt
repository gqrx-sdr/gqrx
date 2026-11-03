add_executable(test_tx test_tx.cpp)

if(Qt6_FOUND)
    set_property(TARGET test_tx PROPERTY CXX_STANDARD 17)
else()
    set_property(TARGET test_tx PROPERTY CXX_STANDARD 14)
endif()

add_test(NAME test_tx COMMAND test_tx)
