target_sources(${PROJECT_SLUG} PRIVATE
    python_engine.cpp
    register_machinery.cpp
)

message(STATUS ${PYTHON_INCLUDE_DIRS})
target_link_libraries(${PROJECT_SLUG} PUBLIC ${PYTHON_LIBRARIES})

