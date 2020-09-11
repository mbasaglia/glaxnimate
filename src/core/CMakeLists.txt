set(SOURCES

app_info.cpp

command/structure_commands.cpp

io/lottie/lottie_format.cpp
io/lottie/lottie_html_format.cpp
io/lottie/tgs_format.cpp
io/lottie/cbor_write_json.cpp
io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/base.cpp
io/svg/inkscape_svg.cpp


model/main_composition.cpp
model/composition.cpp
model/document.cpp
model/document_node.cpp
model/layers/layer.cpp
model/layers/empty_layer.cpp
model/layers/shape_layer.cpp
model/layers/solid_color_layer.cpp
model/object.cpp
model/transform.cpp
model/shapes/shape.cpp
model/factory.cpp
model/animation/keyframe_transition.cpp
model/animation/animatable.cpp
model/shapes/fill.cpp
model/shapes/rect.cpp
model/shapes/group.cpp
model/shapes/ellipse.cpp
model/shapes/path.cpp
model/shapes/stroke.cpp

utils/gzip.cpp
)

if(${PYTHON_ENABLED})
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-attributes WTF_IS_THIS_WARNING)
    if ( ${WTF_IS_THIS_WARNING} )
        SET(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} -Wno-attributes)
    endif()

    list(APPEND SOURCES python.cpp)
endif()

find_package(ZLIB)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)

add_library(${LIB_NAME} OBJECT ${SOURCES})
target_link_libraries(${LIB_NAME} PUBLIC QtAppSetup ZLIB::ZLIB)
set_property(TARGET ${LIB_NAME} APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")
target_include_directories(${LIB_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
