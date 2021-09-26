#ifndef EXTERNALFONTWIDGET_H
#define EXTERNALFONTWIDGET_H

#include <memory>
#include <QWidget>

#include "model/font/custom_font.hpp"

namespace glaxnimate::gui::font {

class ExternalFontWidget : public QWidget
{
    Q_OBJECT

public:
    ExternalFontWidget(QWidget* parent = nullptr);
    ~ExternalFontWidget();

    void set_font_size(double size);

    model::CustomFont custom_font() const;
    const QFont& selected_font() const;

protected:
    void changeEvent ( QEvent* e ) override;
    void showEvent(QShowEvent * event) override;

private slots:
    void url_from_file();
    void load_url();
    void url_changed(const QString& url);

signals:
    void font_changed(const QFont& font);

private:
    class Private;
    std::unique_ptr<Private> d;
};


} // namespace glaxnimate::gui::font

#endif // EXTERNALFONTWIDGET_H
