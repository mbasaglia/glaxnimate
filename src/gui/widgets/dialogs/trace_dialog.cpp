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
#include <QScreen>

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
#include "utils/quantize.hpp"
#include "utils/trace_wrapper.hpp"
#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"
#include "app/widgets/no_close_on_enter.hpp"
#include "glaxnimate_app.hpp"

#include "color_quantization_dialog.hpp"


class glaxnimate::gui::TraceDialog::Private
{
public:
    enum Mode
    {
        Alpha,
        Closest,
        Exact,
        Pixel
    };

    using TraceResult = glaxnimate::utils::trace::TraceWrapper::TraceResult;
    using Preset = glaxnimate::utils::trace::TraceWrapper::Preset;

    utils::trace::TraceWrapper trace_wrapper;
    model::Group* created = nullptr;
    Ui::TraceDialog ui;
    QGraphicsScene scene;
    color_widgets::ColorDelegate delegate;
    qreal zoom = 1;
    QGraphicsRectItem *item_parent_shape;
    QGraphicsRectItem *item_parent_image;
    QGraphicsPixmapItem *item_image;
    app::widgets::NoCloseOnEnter ncoe;
    ColorQuantizationDialog color_options;
    app::settings::WidgetSettingGroup settings;
    QSize image_size;
    bool initialized = false;

    explicit Private(model::Image* image)
        : trace_wrapper(image), image_size(trace_wrapper.size())
    {}

    std::vector<QRgb> colors()
    {
        int count = ui.list_colors->model()->rowCount();
        std::vector<QRgb> colors;
        colors.reserve(count);
        for ( int i = 0; i < count; i++ )
        {
            colors.push_back(ui.list_colors->item(i)->data(Qt::DisplayRole).value<QColor>().rgb());
        }

        return colors;
    }

    std::vector<TraceResult> trace()
    {
        std::vector<TraceResult> result;

        ui.progress_bar->show();
        ui.progress_bar->setValue(0);

        if ( !ui.button_advanced->isChecked() )
        {
            std::vector<QRgb> colors;
            trace_wrapper.trace_preset(Preset(ui.list_presets->currentRow()), ui.spin_posterize->value(), colors, result);
            if ( !colors.empty() )
                set_colors(colors);
        }
        else
        {

            trace_wrapper.options().set_min_area(ui.spin_min_area->value());
            trace_wrapper.options().set_smoothness(ui.spin_smoothness->value() / 100.0);

            switch ( ui.combo_mode->currentIndex() )
            {
                case Mode::Alpha:
                    trace_wrapper.trace_mono(
                        ui.color_mono->color(),
                        ui.check_inverted->isChecked(),
                        ui.spin_alpha_threshold->value(),
                        result
                    );
                    break;
                case Mode::Closest:
                    trace_wrapper.trace_closest(colors(), result);
                    break;
                case Mode::Exact:
                    trace_wrapper.trace_exact(
                        colors(),
                        ui.spin_tolerance->value(),
                        result
                    );
                    break;
                case Mode::Pixel:
                    trace_wrapper.trace_pixel(result);
                    break;
            }
        }

        std::reverse(result.begin(), result.end());
        ui.progress_bar->hide();
        return result;
    }

    qreal outline()
    {
        if ( !ui.button_advanced->isChecked() )
            return ui.list_presets->currentRow() != utils::trace::TraceWrapper::PixelPreset ? 1 : 0;
        if ( ui.combo_mode->currentIndex() == Mode::Closest || ui.combo_mode->currentIndex() == Mode::Exact )
            return ui.spin_outline->value();
        return 0;
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
        item_parent_shape = scene.addRect(QRectF(0, 0, image_size.width(), image_size.height()));
        item_parent_shape->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
        item_parent_shape->setBrush(Qt::NoBrush);
        item_parent_shape->setPen(Qt::NoPen);

        item_parent_image = scene.addRect(QRectF(image_size.width(), 0, 0, image_size.height()));
        item_parent_image->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
        item_parent_image->setBrush(Qt::NoBrush);
        item_parent_image->setPen(Qt::NoPen);

        item_image = new QGraphicsPixmapItem(QPixmap::fromImage(trace_wrapper.image()), item_parent_image);
    }

    void init_settings()
    {
        settings.add(ui.spin_tolerance, "internal", "trace_dialog_");
        settings.add(ui.spin_outline, "internal", "trace_dialog_");
        settings.add(ui.spin_smoothness, "internal", "trace_dialog_");
        settings.add(ui.spin_alpha_threshold, "internal", "trace_dialog_");
        settings.add(ui.spin_min_area, "internal", "trace_dialog_");
        settings.add(ui.spin_posterize, "internal", "trace_dialog_");
        settings.add(ui.button_advanced, "internal", "trace_dialog_");
        settings.define();
        color_options.init_settings();
    }

    void save_settings()
    {
        settings.save();
        color_options.save_settings();
    }

    void reset_settings()
    {
        settings.reset();
        color_options.reset_settings();
    }

    void init_colors()
    {
        if ( image_size.width() < 1024 && image_size.height() < 1024 )
        {
            auto colors = trace_wrapper.eem_colors();
            for ( QRgb rgb : colors )
                add_color(QColor(rgb));
            ui.spin_color_count->setValue(colors.size());
        }
        else
        {
            ui.spin_color_count->setValue(4);
            auto_colors();
        }

    }

    void set_colors(const std::vector<QRgb>& colors)
    {
        while ( ui.list_colors->model()->rowCount() )
            ui.list_colors->model()->removeRow(0);

        for ( QRgb rgb : colors )
            add_color(QColor(rgb));
    }

    void auto_colors()
    {
        int n_colors = ui.spin_color_count->value();
        if ( n_colors )
            set_colors(color_options.quantize(trace_wrapper.image(), n_colors));
        else
            set_colors({});
    }

    void auto_preset()
    {
        ui.list_presets->setCurrentRow(trace_wrapper.preset_suggestion());
    }

#ifdef Q_OS_ANDROID
    void android_ui(TraceDialog* parent)
    {
        ui.button_quantize_options->hide();
        ui.mode_container->hide();
        ui.button_help->hide();
        ui.button_defaults->hide();
        ui.preview_slider->hide();
        ui.spin_preview_zoom->hide();
        ui.label_preview_zoom->hide();
        ui.group_preview_layout->setMargin(0);
        ui.button_detect_colors->setText("Auto colors");
        ui.gridLayout->addWidget(ui.button_detect_colors, 3, 0, 1, 2);
        ui.layout_buttons->insertWidget(0, ui.button_update_preview);

        for ( auto w : parent->findChildren<QSpinBox*>() )
            w->setMaximumHeight(40);
        for ( auto w : parent->findChildren<QDoubleSpinBox*>() )
            w->setMaximumHeight(40);

        for ( auto w : parent->findChildren<QLayout*>() )
            w->setMargin(0);

        zoom = 1.05;

        connect(
            QGuiApplication::primaryScreen(),
            &QScreen::primaryOrientationChanged,
            parent,
            [this]{screen_rotation();}
        );
        screen_rotation();
    }

    void screen_rotation()
    {
        auto scr = QApplication::primaryScreen();
        if ( scr->size().height() > scr->size().width() )
        {
            ui.layout_main->setDirection(QBoxLayout::TopToBottom);
            ui.widget_controls->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        }
        else
        {
            ui.layout_main->setDirection(QBoxLayout::LeftToRight);
            ui.widget_controls->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        }
    }
#endif
};

glaxnimate::gui::TraceDialog::TraceDialog(model::Image* image, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>(image))
{
    d->ui.setupUi(this);
    d->init_scene();
    connect(&d->trace_wrapper, &utils::trace::TraceWrapper::progress_max_changed, d->ui.progress_bar, &QProgressBar::setMaximum);
    connect(&d->trace_wrapper, &utils::trace::TraceWrapper::progress_changed, d->ui.progress_bar, &QProgressBar::setValue);

    d->ui.preview->setScene(&d->scene);
    d->ui.spin_min_area->setValue(qMax(d->trace_wrapper.options().min_area(), d->image_size.width() / 32));
    d->ui.spin_smoothness->setValue(d->trace_wrapper.options().smoothness() * 100);
    d->ui.progress_bar->hide();

    d->delegate.setSizeHintForColor({24, 24});
    d->ui.list_colors->setItemDelegate(&d->delegate);

    d->ui.combo_mode->setCurrentIndex(Private::Closest);
    d->ui.button_defaults->setVisible(false);
    d->init_settings();

    d->ui.list_presets->item(utils::trace::TraceWrapper::ComplexPreset)->setIcon(QPixmap(GlaxnimateApp::instance()->data_file("images/trace/complex.jpg")));
    d->ui.list_presets->item(utils::trace::TraceWrapper::FlatPreset)->setIcon(QPixmap(GlaxnimateApp::instance()->data_file("images/trace/flat.png")));
    d->ui.list_presets->item(utils::trace::TraceWrapper::PixelPreset)->setIcon(QPixmap(GlaxnimateApp::instance()->data_file("images/trace/pixel.png")));

    if ( d->image_size.width() > 128 || d->image_size.height() > 128 )
    {
        auto item = static_cast<QStandardItemModel*>(d->ui.combo_mode->model())->item(Private::Pixel);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

        auto preset_item = d->ui.list_presets->item(utils::trace::TraceWrapper::PixelPreset);
        preset_item->setFlags(preset_item->flags() & ~Qt::ItemIsEnabled);
    }

    if ( d->ui.button_advanced->isChecked() )
        d->init_colors();
    else
        d->auto_preset();

    d->initialized = true;
    update_preview();

    d->ui.preview->setBackgroundBrush(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));

    installEventFilter(&d->ncoe);

    connect(this, &QDialog::accepted, this, [this]{ d->save_settings(); });

#ifdef Q_OS_ANDROID
    d->android_ui(this);
#endif

}

glaxnimate::gui::TraceDialog::~TraceDialog() = default;

void glaxnimate::gui::TraceDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::gui::TraceDialog::update_preview()
{
    if ( !d->initialized )
        return;

    for ( auto ch : d->item_parent_shape->childItems() )
        delete ch;

    for ( const auto& result : d->trace() )
    {
        if ( !result.bezier.beziers().empty() )
        {
            QPen pen = Qt::NoPen;
            qreal pen_width = d->outline();
            if ( pen_width > 0 )
                pen = QPen(result.color, pen_width);

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

void glaxnimate::gui::TraceDialog::apply()
{
    auto trace = d->trace();
    d->created = d->trace_wrapper.apply(trace, d->outline());
    accept();
}

glaxnimate::model::DocumentNode * glaxnimate::gui::TraceDialog::created() const
{
    return d->created;
}

void glaxnimate::gui::TraceDialog::change_mode(int mode)
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


void glaxnimate::gui::TraceDialog::add_color()
{
    d->add_color();
    d->ui.spin_color_count->setValue(d->ui.list_colors->model()->rowCount());
}

void glaxnimate::gui::TraceDialog::remove_color()
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

void glaxnimate::gui::TraceDialog::auto_colors()
{
    d->auto_colors();
    update_preview();
}

void glaxnimate::gui::TraceDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    d->fit_view();
}

void glaxnimate::gui::TraceDialog::zoom_preview(qreal percent)
{
    qreal scale = percent / 100 / d->zoom;
    d->ui.preview->scale(scale, scale);
    d->zoom = percent / 100;
    d->rescale_preview_background();
}

void glaxnimate::gui::TraceDialog::show_help()
{
    QUrl docs = AppInfo::instance().url_docs();
    docs.setPath("/manual/ui/dialogs/");
    docs.setFragment("trace-bitmap");
    QDesktopServices::openUrl(docs);
}

void glaxnimate::gui::TraceDialog::preview_slide(int percent)
{
    qreal splitpoint = d->image_size.width() * percent / 100.0;
    d->item_parent_shape->setRect(0, 0, splitpoint, d->image_size.height());
    d->item_parent_image->setRect(splitpoint, 0, d->image_size.width() - splitpoint, d->image_size.height());
}

void glaxnimate::gui::TraceDialog::reset_settings()
{
    d->reset_settings();
    d->ui.check_inverted->setChecked(false);
    d->ui.combo_mode->setCurrentIndex(Private::Closest);
}

void glaxnimate::gui::TraceDialog::color_options()
{
    d->color_options.exec();
}

void glaxnimate::gui::TraceDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    update_preview();
}

void glaxnimate::gui::TraceDialog::toggle_advanced(bool advanced)
{
    d->ui.stacked_preset_advanced->setCurrentIndex(int(advanced));
    d->ui.button_defaults->setVisible(advanced);
    if ( !advanced )
    {
        d->auto_preset();
        update_preview();
    }
    else
    {
        d->ui.spin_min_area->setValue(16);
        d->ui.spin_smoothness->setValue(75);
        d->ui.spin_color_count->setValue(d->ui.list_colors->count());
        switch ( d->ui.list_presets->currentRow() )
        {
            case Private::Preset::ComplexPreset:
            case Private::Preset::FlatPreset:
                d->ui.combo_mode->setCurrentIndex(Private::Closest);
                break;
            case Private::Preset::PixelPreset:
                d->ui.combo_mode->setCurrentIndex(Private::Pixel);
                break;
        }
    }
}
