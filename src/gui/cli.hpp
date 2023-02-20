/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QApplication>

#include "app/cli.hpp"
#include "glaxnimate_app.hpp"

namespace glaxnimate::gui {


app::cli::ParsedArguments parse_cli(const QStringList& args);
void cli_main(gui::GlaxnimateApp& app, app::cli::ParsedArguments& args);


} // namespace glaxnimate::gui
