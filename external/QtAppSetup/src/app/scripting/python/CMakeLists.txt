target_sources(${PROJECT_SLUG} PRIVATE
    python_engine.cpp
    register_machinery.cpp
)

message(STATUS "PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS}")
message(STATUS "PYTHON_LIBRARIES ${PYTHON_LIBRARIES}")
message(STATUS "Python_INCLUDE_DIRS ${Python_INCLUDE_DIRS}")
message(STATUS "Python_LIBRARIES ${Python_LIBRARIES}")
target_link_libraries(${PROJECT_SLUG} PUBLIC ${PYTHON_LIBRARIES})

