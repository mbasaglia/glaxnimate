target_sources(${PROJECT_SLUG} PRIVATE
    python_engine.cpp
    register_machinery.cpp
)

message(STATUS "Python3_INCLUDE_DIRS ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARIES ${Python3_LIBRARIES}")
message(STATUS "Python3_EXECUTABLE ${Python3_EXECUTABLE}")
target_link_libraries(${PROJECT_SLUG} PUBLIC ${Python3_LIBRARIES})

