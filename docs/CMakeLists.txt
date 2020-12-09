
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.in.yml" "${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.yml")

add_custom_target(
    docs
    COMMAND "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/extensions/" mkdocs build -f "${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.yml" -d "${CMAKE_CURRENT_BINARY_DIR}/site"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.yml"
)

add_custom_target(
    docs_serve
    COMMAND "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/extensions/" mkdocs serve -f "${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.yml"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mkdocs.yml"
)

if(PYTHON_PIP)
    add_custom_target(
        docs_depends_install
        COMMAND ${PYTHON_PIP} install mkdocs mkdocs-cinder git+https://github.com/bmcorser/fontawesome-markdown.git@master
    )
endif()


add_custom_target(
    docs_build_python
    COMMAND PYTHONPATH=${CMAKE_BINARY_DIR}/py_module/build/lib ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/build_py_doc.py" "${CMAKE_CURRENT_SOURCE_DIR}/docs/contributing/scripting/python_reference.md" "${CMAKE_CURRENT_BINARY_DIR}/json_reference.md"
    COMMAND cat "${CMAKE_CURRENT_SOURCE_DIR}/json_reference.in.md" "${CMAKE_CURRENT_BINARY_DIR}/json_reference.md" >"${CMAKE_CURRENT_SOURCE_DIR}/docs/contributing/json_reference.md"
    BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/contributing/scripting/python_reference.md" "${CMAKE_CURRENT_SOURCE_DIR}/docs/contributing/json_reference.md"
    DEPENDS glaxnimate_python
)
