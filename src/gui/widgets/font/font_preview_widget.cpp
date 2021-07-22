#include "font_preview_widget.hpp"
#include "ui_font_preview_widget.h"

#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsTextItem>

using namespace glaxnimate::gui;
using namespace glaxnimate;

class font::FontPreviewWidget::Private
{
public:
    Ui::FontPreviewWidget ui;
    QGraphicsTextItem item;
    QGraphicsScene scene;
    qreal scale = 1;

    QString default_text() const
    {
        return tr("The quick brown fox jumps over the lazy dog");
    }

    void fit()
    {
        ui.view->fitInView(&item, Qt::KeepAspectRatio);
        scale = qBound(0.01, ui.view->transform().m11(), 1.);
        ui.spin_zoom->setValue(scale * 100);
    }
};


font::FontPreviewWidget::FontPreviewWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->ui.view->setScene(&d->scene);
    d->scene.addItem(&d->item);
    d->item.setPlainText(d->default_text());
    d->item.setTextInteractionFlags(Qt::TextEditorInteraction);
    d->item.setFocus();
}

font::FontPreviewWidget::~FontPreviewWidget()
{
    d->scene.removeItem(&d->item);
}

void font::FontPreviewWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void font::FontPreviewWidget::set_font(const QFont& font)
{
    d->item.setFont(font);
    d->fit();
}

void font::FontPreviewWidget::set_text(const QString& text, bool fallback_to_default)
{
    if ( fallback_to_default && text.isEmpty() )
        d->item.setPlainText(d->default_text());
    else
        d->item.setPlainText(text);
}

void font::FontPreviewWidget::zoom_changed(double zoom)
{
    double factor = zoom / 100 / d->scale;
    d->ui.view->scale(factor, factor);
    d->scale = zoom / 100;
    d->ui.view->centerOn(&d->item);
}

void font::FontPreviewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    d->fit();
}

void font::FontPreviewWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    d->fit();
}

