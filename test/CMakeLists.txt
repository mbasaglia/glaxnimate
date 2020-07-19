message(STATUS "Test targets enabled")
set(SOURCE_BASE_DIR ${CMAKE_SOURCE_DIR}/src)
set(COVERAGE_TOOL "gcovr")
include(testing)

include_directories(${CMAKE_SOURCE_DIR}/src)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")


function(test_case test_name)
    add_executable(${test_name} EXCLUDE_FROM_ALL ${test_name}.cpp ${ARGN})
    target_link_libraries(${test_name} PRIVATE Qt5::Test Qt5::Widgets QtColorWidgets)
    add_test (NAME ${test_name} COMMAND ${test_name} -silent)
    add_dependencies(tests_compile ${test_name})
endfunction(test_case)


test_case(test_property ../src/model/object.cpp)
