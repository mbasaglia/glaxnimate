#include "startup_dialog.hpp"
#include "ui_startup_dialog.h"

#include <QEvent>

#include "app/settings/settings.hpp"
#include "settings/document_templates.hpp"

class StartupDialog::Private
{
public:
    Ui::StartupDialog ui;

    model::FrameTime duration_frames()
    {
        if ( ui.combo_time_units->currentIndex() == 0 )
            return ui.spin_duration->value();
        return ui.spin_duration->value() * ui.spin_fps->value();
    }

    float duration(model::FrameTime time, float fps)
    {
        if ( ui.combo_time_units->currentIndex() == 0 || fps <= 0 )
            return time;
        return time / fps;
    }

    QString time_suffix()
    {
        if ( ui.combo_time_units->currentIndex() == 0 )
            return "f";
        return "'";
    }

    QString format_duration(const settings::DocumentTemplate& templ)
    {
        return QString::number(duration(templ.duration(), templ.fps())) + time_suffix();
    }

    void adjust_duration_spin()
    {
        ui.spin_duration->setSuffix(time_suffix());
        if ( ui.combo_time_units->currentIndex() == 0 )
            ui.spin_duration->setDecimals(0);
        else
            ui.spin_duration->setDecimals(2);
    }

    void apply_to_document(model::Document* doc)
    {
        doc->main()->width.set(ui.spin_size->x());
        doc->main()->height.set(ui.spin_size->y());
        doc->main()->fps.set(ui.spin_fps->value());
        doc->main()->animation->last_frame.set(duration_frames());
    }
};

StartupDialog::StartupDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    reload_presets();
    connect(&settings::DocumentTemplates::instance(), &settings::DocumentTemplates::loaded, this, &StartupDialog::reload_presets);
    connect(d->ui.button_open_browse, &QPushButton::clicked, this, &StartupDialog::open_browse);

    d->ui.spin_size->enable_ratio_lock();
    d->ui.spin_size->set_decimals(0);
    d->ui.spin_size->set_value(
        app::settings::get<int>("defaults", "width"),
        app::settings::get<int>("defaults", "height")
    );
    float fps = app::settings::get<float>("defaults", "fps");
    d->ui.spin_fps->setValue(fps);
    d->adjust_duration_spin();
    d->ui.spin_duration->setValue(app::settings::get<float>("defaults", "duration"));

    for ( const auto& recent : app::settings::get<QStringList>("open_save", "recent_files") )
    {
        QFileInfo finfo(recent);
        if ( !finfo.exists() || !finfo.isReadable() )
            continue;
        auto item = new QListWidgetItem(QIcon::fromTheme("document-open-recent"), finfo.fileName());
        item->setData(Qt::ToolTipRole, recent);
        d->ui.view_recent->addItem(item);
    }

    d->ui.view_presets->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for ( int i = 1; i < d->ui.view_presets->columnCount(); i++ )
    {
        d->ui.view_presets->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    d->ui.check_show_at_startup->setChecked(app::settings::get<bool>("ui", "startup_dialog"));
}

StartupDialog::~StartupDialog() = default;

void StartupDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void StartupDialog::reload_presets()
{
    d->ui.view_presets->clearContents();
    int row = 0;
    for ( const auto& templ : settings::DocumentTemplates::instance().templates() )
    {
        d->ui.view_presets->insertRow(row);
        int col = 0;
        d->ui.view_presets->setItem(row, col++, new QTableWidgetItem(QIcon::fromTheme("document-new-from-template"), templ.name()));
        d->ui.view_presets->setItem(row, col++, new QTableWidgetItem(QString("%1x%2").arg(templ.size().width()).arg(templ.size().height())));
        d->ui.view_presets->setItem(row, col++, new QTableWidgetItem(QString("%1 fps").arg(templ.fps())));
        d->ui.view_presets->setItem(row, col++, new QTableWidgetItem(templ.aspect_ratio()));
        d->ui.view_presets->setItem(row, col++, new QTableWidgetItem(d->format_duration(templ)));
        row++;
    }
}

void StartupDialog::select_preset(const QModelIndex& index)
{
    const auto& templ = settings::DocumentTemplates::instance().templates()[index.row()];
    d->ui.spin_size->set_value(templ.size());
    d->ui.spin_fps->setValue(templ.fps());
    d->ui.spin_duration->setValue(d->duration(templ.duration(), templ.fps()));
}

void StartupDialog::click_recent(const QModelIndex& index)
{
    emit open_recent(index.data(Qt::ToolTipRole).toString());
    reject();
}

std::unique_ptr<model::Document> StartupDialog::create() const
{
    auto index = d->ui.view_presets->currentIndex();
    if ( index.isValid() )
    {
        const auto& templ = settings::DocumentTemplates::instance().templates()[index.row()];
        bool ok = false;
        auto doc = templ.create(&ok);
        if ( ok )
        {
            d->apply_to_document(doc.get());
            return doc;
        }
    }

    auto doc = std::make_unique<model::Document>("");
    d->apply_to_document(doc.get());

    auto layer = std::make_unique<model::Layer>(doc.get());
    layer->animation->last_frame.set(d->duration_frames());
    layer->name.set(layer->type_name_human());
    QPointF pos(
        doc->main()->width.get() / 2.0,
        doc->main()->height.get() / 2.0
    );
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    doc->main()->shapes.insert(std::move(layer), 0);

    return doc;
}

void StartupDialog::update_time_units()
{
    d->adjust_duration_spin();

    // seconds * fps = frames
    float mult = d->ui.spin_fps->value();

    // fps -> seconds
    if ( d->ui.combo_time_units->currentIndex() == 1 )
        mult = 1/mult;

    d->ui.spin_duration->setValue(d->ui.spin_duration->value() * mult);

    int row = 0;
    for ( const auto& templ : settings::DocumentTemplates::instance().templates() )
    {
        d->ui.view_presets->item(row, 4)->setData(Qt::DisplayRole, d->format_duration(templ));
        row++;
    }
}

void StartupDialog::update_startup_enabled(bool checked)
{
    app::settings::set("ui", "startup_dialog", checked);
}
