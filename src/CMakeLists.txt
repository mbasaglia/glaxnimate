add_subdirectory(core)
add_subdirectory(gui)

add_executable(${PROJECT_SLUG})
target_link_libraries(${PROJECT_SLUG} ${LIB_NAME} ${LIB_NAME_GUI} QtAppSetup)
install(TARGETS ${PROJECT_SLUG} RUNTIME DESTINATION bin)
