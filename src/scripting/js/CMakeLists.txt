
target_sources(glaxnimate PRIVATE
    js_engine.cpp
)

target_link_libraries(glaxnimate PUBLIC ${Qt5Qml_LIBRARIES})

