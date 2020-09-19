include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Werror HAS_WERROR)
if ( ${HAS_WERROR} )
    SET(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
endif()

add_subdirectory(core)
add_subdirectory(gui)

add_executable(${PROJECT_SLUG} WIN32)
target_link_libraries(${PROJECT_SLUG} ${LIB_NAME} ${LIB_NAME_GUI} QtAppSetup)
install(TARGETS ${PROJECT_SLUG} DESTINATION bin)
