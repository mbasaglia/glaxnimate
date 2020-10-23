add_subdirectory(core)
add_subdirectory(gui)

add_executable(${PROJECT_SLUG} WIN32 core/python/embed.cpp)
target_link_libraries(${PROJECT_SLUG} ${LIB_NAME_CORE} ${LIB_NAME_GUI} QtAppSetup)
install(TARGETS ${PROJECT_SLUG} DESTINATION bin)
