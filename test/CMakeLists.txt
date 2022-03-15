message(STATUS "Test targets enabled")
set(SOURCE_BASE_DIR ${CMAKE_SOURCE_DIR}/src)
set(COVERAGE_TOOL "gcovr")
set(COVERAGE_FILTER_PATTERNS ".*/src/core/.*")
include(testing)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")


function(test_case test_name)
    add_executable(${test_name} EXCLUDE_FROM_ALL ${test_name}.cpp ${ARGN})
    target_link_libraries(${test_name} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets QtColorWidgets QtAppSetup)
    add_test (NAME ${test_name} COMMAND ${test_name} -silent)
    add_dependencies(tests_compile ${test_name})
    set_property(TARGET ${test_name} APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")
endfunction(test_case)

include_directories(${CMAKE_SOURCE_DIR}/src/core)

test_case(test_property)
target_link_libraries(test_property PRIVATE ${LIB_NAME_CORE})

test_case(test_bezier_solver)
test_case(test_keyframe_transition
../src/core/model/animation/keyframe_transition.cpp
)

test_case(test_trim_path)
target_link_libraries(test_trim_path PRIVATE ${LIB_NAME_CORE})


test_case(test_trace)
target_link_libraries(test_trace PRIVATE ${LIB_NAME_CORE})


test_case(test_segmentation
../src/core/trace/segmentation.cpp
../src/core/trace/color.cpp
)


test_case(test_trace_gradient
../src/core/trace/segmentation.cpp
../src/core/trace/color.cpp
../src/core/trace/gradient.cpp
)


test_case(test_cluster_merge
../src/core/trace/segmentation.cpp
../src/core/trace/quantize.cpp
../src/core/trace/color.cpp
../src/core/trace/gradient.cpp
)
