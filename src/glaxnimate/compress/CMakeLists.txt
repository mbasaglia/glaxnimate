set(SOURCES
gzip.cpp
)

if ( NOT ANDROID )
    list(APPEND SOURCES
        tar.cpp
    )
    find_package(LibArchive REQUIRED)
    include_directories(${LibArchive_INCLUDE_DIRS})
endif()

find_package(ZLIB REQUIRED)
add_library(glaxnimate_compress OBJECT ${SOURCES})

target_link_libraries(glaxnimate_compress PUBLIC
    ZLIB::ZLIB
    Qt${QT_VERSION_MAJOR}::Core
)

if ( NOT ANDROID )
    target_link_libraries(glaxnimate_compress PUBLIC ${LibArchive_LIBRARIES})
endif()
