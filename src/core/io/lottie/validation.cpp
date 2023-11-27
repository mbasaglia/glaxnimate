/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "validation.hpp"
#include "model/visitor.hpp"
#include "model/shapes/image.hpp"
#include "model/assets/composition.hpp"


using namespace glaxnimate;
using namespace glaxnimate::io::lottie;

void glaxnimate::io::lottie::ValidationVisitor::on_visit_document(model::Document*, model::Composition* main)
{
    if ( !main )
        return;

    if ( fixed_size.isValid() )
    {
        qreal width = main->height.get();
        if ( width != fixed_size.width() )
            fmt->error(LottieFormat::tr("Invalid width: %1, should be %2").arg(width).arg(fixed_size.width()));

        qreal height = main->height.get();
        if ( height != fixed_size.height() )
            fmt->error(LottieFormat::tr("Invalid height: %1, should be %2").arg(height).arg(fixed_size.height()));
    }

    if ( !allowed_fps.empty() )
    {
        qreal fps = main->fps.get();
        if ( std::find(allowed_fps.begin(), allowed_fps.end(), fps) == allowed_fps.end() )
        {
            QStringList allowed;
            for ( auto f : allowed_fps )
                allowed.push_back(QString::number(f));

            fmt->error(LottieFormat::tr("Invalid fps: %1, should be %2").arg(fps).arg(allowed.join(" or "_qs)));
        }
    }

    if ( max_frames > 0 )
    {
        auto duration = main->animation->duration();
        if ( duration > max_frames )
            fmt->error(LottieFormat::tr("Too many frames: %1, should be less than %2").arg(duration).arg(max_frames));
    }
}


namespace {

class DiscordVisitor : public ValidationVisitor
{
public:
    explicit DiscordVisitor(LottieFormat* fmt)
        : ValidationVisitor(fmt)
    {
        allowed_fps.push_back(60);
        fixed_size = QSize(320, 320);
    }

private:
    void on_visit(model::DocumentNode * node) override
    {
        if ( qobject_cast<model::Image*>(node) )
        {
            show_error(node, LottieFormat::tr("Images are not supported"), app::log::Error);
        }
    }
};

} // namespace

void glaxnimate::io::lottie::validate_discord(model::Document* document, model::Composition* main, glaxnimate::io::lottie::LottieFormat* format)
{
    DiscordVisitor(format).visit(document, main);
}

