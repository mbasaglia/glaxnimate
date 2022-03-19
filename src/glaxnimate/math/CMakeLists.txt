set(SOURCES
geom.cpp
ellipse_solver.cpp
bezier/bezier.cpp
bezier/point.cpp
bezier/operations.cpp
bezier/cubic_struts.cpp
bezier/meta.cpp
)

add_library(glaxnimate_math OBJECT ${SOURCES})
target_link_libraries(glaxnimate_math PUBLIC
    Qt${QT_VERSION_MAJOR}::Gui
)
