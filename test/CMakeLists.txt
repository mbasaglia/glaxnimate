message(STATUS "Test targets enabled")
set(SOURCE_BASE_DIR ${CMAKE_SOURCE_DIR}/src)
set(COVERAGE_TOOL "gcovr")
include(testing)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR}/bin)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")


function(test_case test_name)
    add_executable(${test_name} EXCLUDE_FROM_ALL ${test_name}.cpp ${ARGN})
    target_link_libraries(${test_name} PRIVATE Qt5::Test Qt5::Widgets QtColorWidgets)
    add_test (NAME ${test_name} COMMAND ${test_name} -silent)
    add_dependencies(tests_compile ${test_name})
endfunction(test_case)


# set(TEST_SOURCES ${ALL_SOURCES})
# list(REMOVE_ITEM TEST_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

test_case(test_property
../src/model/object.cpp
../src/model/layers.cpp
../src/model/document.cpp
../src/model/document_node.cpp
../src/model/composition.cpp
../src/model/animation.cpp
../src/model/graphics/handle.cpp
../src/model/graphics/document_node_graphics_item.cpp
../src/io/glaxnimate/glaxnimate_format.cpp
../src/io/glaxnimate/glaxnimate_importer.cpp
../src/io/base.cpp
../src/app_info.cpp
)
