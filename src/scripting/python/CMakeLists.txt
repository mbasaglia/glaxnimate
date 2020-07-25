target_sources(glaxnimate PRIVATE
    python_engine.cpp
)

message(STATUS ${PYTHON_INCLUDE_DIRS})
target_link_libraries(glaxnimate PUBLIC ${PYTHON_LIBRARIES})
target_include_directories (glaxnimate PRIVATE
    ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/external/pybind11/include
)

