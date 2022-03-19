set(SOURCES
lottie_format.cpp
lottie_html_format.cpp
tgs_format.cpp
cbor_write_json.cpp
validation.cpp
)

add_library(glaxnimate_lottie OBJECT ${SOURCES})
target_link_libraries(glaxnimate_lottie PUBLIC glaxnimate_core glaxnimate_compress)
