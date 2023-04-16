/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <functional>

#include <QDir>

#include "io/base.hpp"

namespace glaxnimate::io::avd {

class AvdParser
{
private:


public:
    /**
     * \throws SvgParseError on error
     */
    AvdParser(
        QIODevice* device,
        const QDir& resource_path,
        model::Document* document,
        const std::function<void(const QString&)>& on_warning = {},
        ImportExport* io = nullptr,
        QSize forced_size = {},
        model::FrameTime default_time = 180
    );

    ~AvdParser();

    void parse_to_document();

    class Private;
private:
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::io::avd
