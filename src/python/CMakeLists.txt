set(SOURCES
    python.cpp
    miscdefs.cpp
)
add_library(${LIB_NAME_PYTHON} OBJECT ${SOURCES} embed.cpp)
#target_include_directories (${LIB_NAME_PYTHON} PUBLIC
        #${Python3_INCLUDE_DIRS}
        #${CMAKE_SOURCE_DIR}/external/QtAppSetup/external/pybind11/include
#)
target_link_libraries(${LIB_NAME_PYTHON} PUBLIC ${LIB_NAME_CORE} QtAppSetup)

pybind11_add_module(glaxnimate_python EXCLUDE_FROM_ALL ${SOURCES} module.cpp)
set_target_properties(
    glaxnimate_python PROPERTIES
    OUTPUT_NAME ${PROJECT_SLUG}
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/build/lib"
)
target_link_libraries(glaxnimate_python PUBLIC ${LIB_NAME_CORE} QtAppSetup)
add_custom_command(TARGET glaxnimate_python PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/build/lib")


get_target_property(pain1 glaxnimate_python PREFIX)
get_target_property(pain2 glaxnimate_python OUTPUT_NAME)
get_target_property(pain3 glaxnimate_python SUFFIX)
set(MODULE_LIB_NAME "${pain1}${pain2}${pain3}")

configure_file(README.md ${CMAKE_CURRENT_BINARY_DIR}/README.md COPYONLY)
configure_file(setup.in.py ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
configure_file(MANIFEST.in.in ${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in)


if(PYTHON_PIP)
    add_custom_target(
        glaxnimate_python_depends_install
        COMMAND ${PYTHON_PIP} install -r ${CMAKE_CURRENT_SOURCE_DIR}/build-requirements.txt
    )
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(WHEEL_CMD "--plat-name" "manylinux1_x86_64")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(WHEEL_CMD "--plat-name" "win32")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    execute_process(
        COMMAND echo "from distutils.util import get_platform; print(get_platform().replace('.', '_').replace('-', '_')).replace('x_11_x', 'x_11_0_x')"
        COMMAND python3
        OUTPUT_VARIABLE MAC_PY_PLATFORM_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(WHEEL_CMD "--plat-name" ${MAC_PY_PLATFORM_TAG})
else()
    set(WHEEL_CMD)
endif()

add_custom_target(
    glaxnimate_python_wheel
    COMMAND rm -f ./dist/*.whl
    COMMAND ./setup.py build bdist_wheel ${WHEEL_CMD}
    #DEPENDS glaxnimate_python
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(
    glaxnimate_python_upload
    COMMAND ${Python3_EXECUTABLE} -m twine upload dist/*.whl --verbose
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
