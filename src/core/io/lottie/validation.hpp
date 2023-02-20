/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "lottie_format.hpp"
#include "model/visitor.hpp"

namespace glaxnimate::io::lottie {

class ValidationVisitor : public model::Visitor
{
public:
    explicit ValidationVisitor(LottieFormat* fmt) : fmt(fmt) {}

protected:
    void show_error(model::DocumentNode * node, const QString& message, app::log::Severity severity)
    {
        fmt->message(LottieFormat::tr("%1: %2").arg(node->object_name()).arg(message), severity);
    }

    void on_visit(model::Document * document) override;

    LottieFormat* fmt;
    QSize fixed_size;
    std::vector<int> allowed_fps;
    int max_frames = 0;
};

/**
 * \brief Triggers warnings on \p format if \p document isn't suitable for Discord stickers
 */
void validate_discord(model::Document* document, LottieFormat* format);


} // namespace glaxnimate::io::lottie
