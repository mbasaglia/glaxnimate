# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-FileCopyrightText: 2023 Julius Künzel <jk.kdedev@smartlab.uber.space>
# SPDX-License-Identifier: BSD-2-Clause

include(ECMAddTests)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    # ${CMAKE_SOURCE_DIR}/external/QtAppSetup/src
    ${CMAKE_SOURCE_DIR}/src/core
)

set(TESTS_LINK_LIBS
    Qt${QT_MAJOR_VERSION}::Test
    Qt${QT_MAJOR_VERSION}::Widgets
    QtColorWidgets
    QtAppSetup
)

# Single source file tests, linking only to default libs

ecm_add_tests(
    test_bezier_solver.cpp
    test_riff.cpp
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

# Single source file tests, linking to core lib additionally

ecm_add_tests(
    test_property.cpp
    test_trim_path.cpp
    test_trace.cpp
    test_aep_gradient_xml.cpp
    LINK_LIBRARIES ${TESTS_LINK_LIBS} ${LIB_NAME_CORE}
)

# Multi source files tests

ecm_add_test(
    test_bezier_length.cpp
    ../src/core/math/bezier/bezier.cpp
    ../src/core/math/bezier/point.cpp
    ../src/core/math/bezier/bezier_length.cpp
    TEST_NAME test_bezier_length
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

ecm_add_test(
    test_keyframe_transition.cpp
    ../src/core/model/animation/keyframe_transition.cpp
    ../src/core/math/polynomial.cpp
    TEST_NAME test_keyframe_transition
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)

ecm_add_test(
    test_cos_parser.cpp
    ../src/core/io/aep/string_decoder.cpp
    TEST_NAME test_cos_parser
    LINK_LIBRARIES ${TESTS_LINK_LIBS}
)
