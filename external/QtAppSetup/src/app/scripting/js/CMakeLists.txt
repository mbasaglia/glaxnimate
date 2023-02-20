# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
#
# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(${PROJECT_SLUG} PRIVATE
    js_engine.cpp
)

target_link_libraries(${PROJECT_SLUG} PUBLIC ${Qt5Qml_LIBRARIES})

