#include "trace_dialog.hpp"
#include "ui_trace_dialog.h"

#include <QEvent>
#include <QGraphicsScene>

#include "model/defs/bitmap.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/layer.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/image.hpp"
#include "utils/trace.hpp"
#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"

class TraceDialog::Private
{
public:
    struct TraceResult
    {
        QColor color;
        math::bezier::MultiBezier bezier;
    };

    model::Image* image;
    model::Group* created = nullptr;
    QImage source_image;
    Ui::TraceDialog ui;
    QGraphicsScene scene;
    utils::trace::TraceOptions options;

    std::vector<TraceResult> trace()
    {
        options.set_min_area(ui.spin_min_area->value());
        options.set_smoothness(ui.spin_smoothness->value() / 100.0);

        std::vector<TraceResult> result;

        ui.progress_bar->show();
        ui.progress_bar->setValue(0);

        // mono
        result.resize(1);
        ui.progress_bar->setMaximum(100);

        result[0].color = ui.color_mono->color();
        utils::trace::Tracer tracer(source_image, options);
        connect(&tracer, &utils::trace::Tracer::progress, ui.progress_bar, &QProgressBar::setValue);
        tracer.set_progress_range(0, 100);
        tracer.trace(result[0].bezier);

        ui.progress_bar->hide();
        return result;
    }

    void result_to_shapes(model::ShapeListProperty& prop, const TraceResult& result)
    {
        auto fill = std::make_unique<model::Fill>(image->document());
        fill->color.set(result.color);
        prop.insert(std::move(fill));

        for ( const auto& bez : result.bezier.beziers() )
        {
            auto path = std::make_unique<model::Path>(image->document());
            path->shape.set(bez);
            prop.insert(std::move(path));
        }
    }
};

TraceDialog::TraceDialog(model::Image* image, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->image = image;
    d->source_image = image->image->pixmap().toImage();
    d->ui.preview->setScene(&d->scene);
    d->ui.spin_min_area->setValue(d->options.min_area());
    d->ui.spin_smoothness->setValue(d->options.smoothness() * 100);
    d->ui.progress_bar->hide();
}

TraceDialog::~TraceDialog() = default;

void TraceDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void TraceDialog::update_preview()
{
    d->scene.clear();

    for ( const auto& result : d->trace() )
    {
        d->scene.addPath(result.bezier.painter_path(), Qt::NoPen, result.color);
    }

    d->ui.preview->fitInView(d->scene.sceneRect(), Qt::KeepAspectRatio);
}

void TraceDialog::apply()
{
    auto trace = d->trace();

    auto layer = std::make_unique<model::Group>(d->image->document());
    d->created = layer.get();
    layer->name.set(tr("Traced %1").arg(d->image->object_name()));
    layer->transform->copy(d->image->transform.get());

    if ( trace.size() == 1 )
    {
        d->result_to_shapes(layer->shapes, trace[0]);
    }
    else
    {
        for ( const auto& result : trace )
        {
            auto group = std::make_unique<model::Group>(d->image->document());
            group->name.set(result.color.name());
            d->result_to_shapes(group->shapes, result);
            layer->shapes.insert(std::move(group));
        }
    }

    d->image->push_command(new command::AddObject<model::ShapeElement>(
        d->image->owner(), std::move(layer), d->image->position()+1
    ));

    d->created->recursive_rename();

    accept();
}

model::DocumentNode * TraceDialog::created() const
{
    return d->created;
}
