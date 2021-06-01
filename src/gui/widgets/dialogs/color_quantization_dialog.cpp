#include "color_quantization_dialog.hpp"
#include "ui_color_quantization_dialog.h"

#include <QEvent>

#include "utils/quantize.hpp"

#include "app/settings/widget.hpp"

class ColorQuantizationDialog::Private
{
public:
    Ui::ColorQuantizationDialog ui;
};

ColorQuantizationDialog::ColorQuantizationDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

ColorQuantizationDialog::~ColorQuantizationDialog() = default;

void ColorQuantizationDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

std::vector<QRgb> ColorQuantizationDialog::quantize(const QImage& image, int k) const
{
    switch ( d->ui.combo_algo->currentIndex() )
    {
        case 0:
            return utils::quantize::k_means(
                image,
                k,
                d->ui.spin_means_iterations->value(),
                utils::quantize::KMeansMatch(d->ui.combo_means_match->currentIndex())
           );
        case 1:
            return utils::quantize::k_modes(image, k);
        case 2:
            return utils::quantize::octree(image, k);
    }
    return {};
}

void ColorQuantizationDialog::init_settings()
{
    app::settings::WidgetSetting(d->ui.combo_algo, "internal", "color_quantization_dialog").define();
    app::settings::WidgetSetting(d->ui.spin_means_iterations, "internal", "color_quantization_dialog").define();
    app::settings::WidgetSetting(d->ui.combo_means_match, "internal", "color_quantization_dialog").define();
}

void ColorQuantizationDialog::save_settings()
{
    app::settings::WidgetSetting(d->ui.combo_algo, "internal", "color_quantization_dialog").save();
    app::settings::WidgetSetting(d->ui.spin_means_iterations, "internal", "color_quantization_dialog").save();
    app::settings::WidgetSetting(d->ui.combo_means_match, "internal", "color_quantization_dialog").save();
}

void ColorQuantizationDialog::reset_settings()
{
    app::settings::WidgetSetting(d->ui.combo_algo, "internal", "color_quantization_dialog").reset();
    app::settings::WidgetSetting(d->ui.spin_means_iterations, "internal", "color_quantization_dialog").reset();
    app::settings::WidgetSetting(d->ui.combo_means_match, "internal", "color_quantization_dialog").reset();
}
