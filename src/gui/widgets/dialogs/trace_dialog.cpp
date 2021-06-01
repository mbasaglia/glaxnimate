#include "trace_dialog.hpp"
#include "ui_trace_dialog.h"

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QStandardItemModel>
#include <QDesktopServices>

#include <QtColorWidgets/ColorDelegate>

#include "app/application.hpp"
#include "app/settings/widget.hpp"
#include "app_info.hpp"

#include "model/assets/bitmap.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/layer.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/stroke.hpp"
#include "model/shapes/image.hpp"
#include "model/shapes/rect.hpp"
#include "utils/trace.hpp"
#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"
#include "app/widgets/no_close_on_enter.hpp"

#include "color_quantization_dialog.hpp"

class TraceDialog::Private
{
public:
    struct TraceResult
    {
        QColor color;
        math::bezier::MultiBezier bezier;
        std::vector<QRectF> rects;
    };

    enum Mode
    {
        Alpha,
        Closest,
        Exact,
        Pixel
    };

    model::Image* image;
    model::Group* created = nullptr;
    QImage source_image;
    Ui::TraceDialog ui;
    QGraphicsScene scene;
    utils::trace::TraceOptions options;
    color_widgets::ColorDelegate delegate;
    qreal zoom = 1;
    QGraphicsRectItem *item_parent_shape;
    QGraphicsRectItem *item_parent_image;
    QGraphicsPixmapItem *item_image;
    app::widgets::NoCloseOnEnter ncoe;
    ColorQuantizationDialog color_options;

    void trace_mono(std::vector<TraceResult>& result)
    {
        result.resize(1);
        ui.progress_bar->setMaximum(100);

        result[0].color = ui.color_mono->color();
        utils::trace::Tracer tracer(source_image, options);
        tracer.set_target_alpha(ui.spin_alpha_threshold->value(), ui.check_inverted->isChecked());
        connect(&tracer, &utils::trace::Tracer::progress, ui.progress_bar, &QProgressBar::setValue);
        tracer.set_progress_range(0, 100);
        tracer.trace(result[0].bezier);
    }

    void trace_exact(std::vector<TraceResult>& result)
    {
        int count = ui.list_colors->model()->rowCount();
        result.resize(count);
        ui.progress_bar->setMaximum(100 * count);

        for ( int i = 0; i < count; i++ )
        {
            result[i].color = ui.list_colors->item(i)->data(Qt::DisplayRole).value<QColor>();
            utils::trace::Tracer tracer(source_image, options);
            tracer.set_target_color(result[i].color, ui.spin_tolerance->value() * ui.spin_tolerance->value());
            connect(&tracer, &utils::trace::Tracer::progress, ui.progress_bar, &QProgressBar::setValue);
            tracer.set_progress_range(100 * i, 100 * (i+1));
            tracer.trace(result[i].bezier);
        }
    }

    void trace_closest(std::vector<TraceResult>& result)
    {
        int count = ui.list_colors->model()->rowCount();
        result.resize(count);
        ui.progress_bar->setMaximum(100 * count);

        QVector<QRgb> colors;
        colors.reserve(count+1);
        for ( int i = 0; i < count; i++ )
        {
            result[i].color = ui.list_colors->item(i)->data(Qt::DisplayRole).value<QColor>();
            colors.push_back(result[i].color.rgb());
        }
        colors.push_back(qRgba(0, 0, 0, 0));
        QImage converted = source_image.convertToFormat(QImage::Format_Indexed8, colors, Qt::ThresholdDither);

        for ( int i = 0; i < count; i++ )
        {
            utils::trace::Tracer tracer(converted, options);
            tracer.set_target_index(i);
            connect(&tracer, &utils::trace::Tracer::progress, ui.progress_bar, &QProgressBar::setValue);
            tracer.set_progress_range(100 * i, 100 * (i+1));
            tracer.trace(result[i].bezier);
        }
    }

    void trace_pixel(std::vector<TraceResult>& result)
    {
        auto pixdata = utils::trace::trace_pixels(source_image);
        result.reserve(pixdata.size());
        for ( const auto& p : pixdata )
            result.push_back({p.first, {}, p.second});
    }
    
    std::vector<TraceResult> trace()
    {
        options.set_min_area(ui.spin_min_area->value());
        options.set_smoothness(ui.spin_smoothness->value() / 100.0);

        std::vector<TraceResult> result;

        ui.progress_bar->show();
        ui.progress_bar->setValue(0);

        switch ( ui.combo_mode->currentIndex() )
        {
            case Mode::Alpha: trace_mono(result); break;
            case Mode::Closest: trace_closest(result); break;
            case Mode::Exact: trace_exact(result); break;
            case Mode::Pixel: trace_pixel(result); break;
        }


        std::reverse(result.begin(), result.end());
        ui.progress_bar->hide();
        return result;
    }

    bool has_outline()
    {
        return ui.spin_outline->value() > 0 && (ui.combo_mode->currentIndex() == Mode::Closest || ui.combo_mode->currentIndex() == Mode::Exact);
    }

    void result_to_shapes(model::ShapeListProperty& prop, const TraceResult& result)
    {
        auto fill = std::make_unique<model::Fill>(image->document());
        fill->color.set(result.color);
        prop.insert(std::move(fill));

        if ( has_outline() )
        {
            auto stroke = std::make_unique<model::Stroke>(image->document());
            stroke->color.set(result.color);
            stroke->width.set(ui.spin_outline->value());
            prop.insert(std::move(stroke));
        }

        for ( const auto& bez : result.bezier.beziers() )
        {
            auto path = std::make_unique<model::Path>(image->document());
            path->shape.set(bez);
            prop.insert(std::move(path));
        }

        for ( const auto& rect : result.rects )
        {
            auto shape = std::make_unique<model::Rect>(image->document());
            shape->position.set(rect.center());
            shape->size.set(rect.size());
            prop.insert(std::move(shape));
        }
    }

    void add_color(const QColor& c = Qt::black)
    {
        auto item = new QListWidgetItem();
        item->setData(Qt::EditRole, c);
        item->setData(Qt::DisplayRole, c);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui.list_colors->addItem(item);
    }

    void fit_view()
    {
        ui.preview->fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
        ui.preview->scale(zoom, zoom);
        rescale_preview_background();
    }

    void rescale_preview_background()
    {
        QBrush b = ui.preview->backgroundBrush();
        b.setTransform(ui.preview->transform().inverted());
        ui.preview->setBackgroundBrush(b);
    }

    void init_scene()
    {
        item_parent_shape = scene.addRect(QRectF(0, 0, source_image.width(), source_image.height()));
        item_parent_shape->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
        item_parent_shape->setBrush(Qt::NoBrush);
        item_parent_shape->setPen(Qt::NoPen);

        item_parent_image = scene.addRect(QRectF(source_image.width(), 0, 0, source_image.height()));
        item_parent_image->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
        item_parent_image->setBrush(Qt::NoBrush);
        item_parent_image->setPen(Qt::NoPen);

        item_image = new QGraphicsPixmapItem(QPixmap::fromImage(source_image), item_parent_image);
    }

    void init_settings()
    {
        app::settings::WidgetSetting(ui.spin_tolerance, "internal", "trace_dialog").define();
        app::settings::WidgetSetting(ui.spin_outline, "internal", "trace_dialog").define();
        app::settings::WidgetSetting(ui.spin_smoothness, "internal", "trace_dialog").define();
        app::settings::WidgetSetting(ui.spin_alpha_threshold, "internal", "trace_dialog").define();
        color_options.init_settings();
    }

    void save_settings()
    {
        app::settings::WidgetSetting(ui.spin_tolerance, "internal", "trace_dialog").save();
        app::settings::WidgetSetting(ui.spin_outline, "internal", "trace_dialog").save();
        app::settings::WidgetSetting(ui.spin_smoothness, "internal", "trace_dialog").save();
        app::settings::WidgetSetting(ui.spin_alpha_threshold, "internal", "trace_dialog").save();
        color_options.save_settings();
    }

    void reset_settings()
    {
        app::settings::WidgetSetting(ui.spin_tolerance, "internal", "trace_dialog").reset();
        app::settings::WidgetSetting(ui.spin_outline, "internal", "trace_dialog").reset();
        app::settings::WidgetSetting(ui.spin_smoothness, "internal", "trace_dialog").reset();
        app::settings::WidgetSetting(ui.spin_alpha_threshold, "internal", "trace_dialog").reset();
        color_options.reset_settings();
    }

    void auto_colors()
    {
        while ( ui.list_colors->model()->rowCount() )
            ui.list_colors->model()->removeRow(0);

        int n_colors = ui.spin_color_count->value();
        if ( n_colors )
        {
            for ( QRgb rgb : color_options.quantize(source_image, n_colors) )
                add_color(QColor(rgb));
        }
    }
};

TraceDialog::TraceDialog(model::Image* image, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->image = image;
    d->source_image = image->image->pixmap().toImage();
    if ( d->source_image.format() != QImage::Format_RGBA8888 )
        d->source_image = d->source_image.convertToFormat(QImage::Format_RGBA8888);
    d->init_scene();

    d->ui.preview->setScene(&d->scene);
    d->ui.spin_min_area->setValue(qMax(d->options.min_area(), d->source_image.width() / 32));
    d->ui.spin_smoothness->setValue(d->options.smoothness() * 100);
    d->ui.progress_bar->hide();

    d->delegate.setSizeHintForColor({24, 24});
    d->ui.list_colors->setItemDelegate(&d->delegate);

    d->init_settings();
    d->ui.spin_color_count->setValue(4);
    d->auto_colors();

    if ( d->source_image.width() > 128 || d->source_image.height() > 128 )
    {
        auto item = static_cast<QStandardItemModel*>(d->ui.combo_mode->model())->item(Private::Pixel);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }

    d->ui.combo_mode->setCurrentIndex(Private::Closest);

    d->ui.preview->setBackgroundBrush(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));

    installEventFilter(&d->ncoe);

    connect(this, &QDialog::accepted, this, [this]{ d->save_settings(); });

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
    for ( auto ch : d->item_parent_shape->childItems() )
        delete ch;

    for ( const auto& result : d->trace() )
    {
        if ( !result.bezier.beziers().empty() )
        {
            QPen pen = Qt::NoPen;
            if ( d->has_outline() )
                pen = QPen(result.color, d->ui.spin_outline->value());

            auto path = new QGraphicsPathItem(result.bezier.painter_path(), d->item_parent_shape);
            path->setPen(pen);
            path->setBrush(result.color);
        }

        if ( !result.rects.empty() )
        {
            for ( const auto& rect : result.rects )
            {
                auto item = new QGraphicsRectItem(rect, d->item_parent_shape);
                item->setPen(Qt::NoPen);
                item->setBrush(result.color);
            }
        }
    }

    d->fit_view();
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
            group->group_color.set(result.color);
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

void TraceDialog::change_mode(int mode)
{
    if ( mode == Private::Alpha )
        d->ui.stacked_widget->setCurrentIndex(0);
    else if ( mode == Private::Pixel )
        d->ui.stacked_widget->setCurrentIndex(2);
    else
        d->ui.stacked_widget->setCurrentIndex(1);

    d->ui.group_potrace->setEnabled(mode != Private::Pixel);
    d->ui.label_tolerance->setEnabled(mode == Private::Exact);
    d->ui.spin_tolerance->setEnabled(mode == Private::Exact);
}


void TraceDialog::add_color()
{
    d->add_color();
    d->ui.spin_color_count->setValue(d->ui.list_colors->model()->rowCount());
}

void TraceDialog::remove_color()
{
    int curr = d->ui.list_colors->currentRow();
    if ( curr == -1 )
    {
        curr = d->ui.list_colors->model()->rowCount() - 1;
        if ( curr == -1 )
            return;
    }
    d->ui.list_colors->model()->removeRow(curr);
    d->ui.spin_color_count->setValue(d->ui.list_colors->model()->rowCount());
}

void TraceDialog::auto_colors()
{
    d->auto_colors();
    update_preview();
}

void TraceDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    d->fit_view();
}

void TraceDialog::zoom_preview(qreal percent)
{
    qreal scale = percent / 100 / d->zoom;
    d->ui.preview->scale(scale, scale);
    d->zoom = percent / 100;
    d->rescale_preview_background();
}

void TraceDialog::show_help()
{
    QUrl docs = AppInfo::instance().url_docs();
    docs.setPath("/manual/ui/dialogs/");
    docs.setFragment("trace-bitmap");
    QDesktopServices::openUrl(docs);
}

void TraceDialog::preview_slide(int percent)
{
    qreal splitpoint = d->source_image.width() * percent / 100.0;
    d->item_parent_shape->setRect(0, 0, splitpoint, d->source_image.height());
    d->item_parent_image->setRect(splitpoint, 0, d->source_image.width() - splitpoint, d->source_image.height());
}

void TraceDialog::reset_settings()
{
    d->reset_settings();
    d->ui.check_inverted->setChecked(false);
    d->ui.combo_mode->setCurrentIndex(Private::Closest);
}

void TraceDialog::color_options()
{
    d->color_options.exec();
}

void TraceDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    update_preview();
}
