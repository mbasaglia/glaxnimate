
target_sources(${PROJECT_SLUG} PRIVATE
    js_engine.cpp
)

target_link_libraries(${PROJECT_SLUG} PUBLIC ${Qt5Qml_LIBRARIES})

