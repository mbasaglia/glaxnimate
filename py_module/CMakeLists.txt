include_directories(${CMAKE_BUILD_DIR}/src/core)
pybind11_add_module(pyglaxnimate module.cpp)
set_target_properties(pyglaxnimate PROPERTIES OUTPUT_NAME ${PROJECT_SLUG})
target_link_libraries(pyglaxnimate PUBLIC ${LIB_NAME_CORE} QtAppSetup)
