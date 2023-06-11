/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "aep_riff.hpp"
#include "ae_project.hpp"
#include "cos.hpp"
#include "gradient_xml.hpp"

namespace glaxnimate::io::aep {

class AepParser
{
public:
    void parse(const RiffChunk& root);
};

} // namespace glaxnimate::io::aep


