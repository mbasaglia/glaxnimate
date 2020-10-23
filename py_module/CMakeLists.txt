include_directories(${CMAKE_BUILD_DIR}/src/core)
pybind11_add_module(glaxnimate_python module.cpp)
set_target_properties(
    glaxnimate_python PROPERTIES
    OUTPUT_NAME ${PROJECT_SLUG}
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/build/lib"
)
target_link_libraries(glaxnimate_python PUBLIC ${LIB_NAME_CORE} QtAppSetup)

configure_file(README.md ${CMAKE_CURRENT_BINARY_DIR}/README.md COPYONLY)
configure_file(setup.in.py ${CMAKE_CURRENT_BINARY_DIR}/setup.py)


if(PYTHON_PIP)
    add_custom_target(
        glaxnimate_python_depends_install
        COMMAND ${PYTHON_PIP} install -r ${CMAKE_CURRENT_SOURCE_DIR}/build-requirements.txt
    )
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(WHEEL_CMD "--plat-name" "manylinux1_x86_64")
else()
    set(WHEEL_CMD)
endif()

add_custom_target(
    glaxnimate_python_wheel
    COMMAND rm -f ./dist/*.whl
    COMMAND ./setup.py build bdist_wheel ${WHEEL_CMD}
    DEPENDS glaxnimate_python
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(
    glaxnimate_python_upload
    COMMAND ${PYTHON_EXEC} -m twine upload dist/*.whl
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
