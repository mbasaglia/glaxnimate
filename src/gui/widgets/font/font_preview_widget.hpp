#ifndef FONT_FONTPREVIEWWIDGET_H
#define FONT_FONTPREVIEWWIDGET_H

#include <memory>
#include <QWidget>

namespace glaxnimate::gui::font {

class FontPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    FontPreviewWidget(QWidget* parent = nullptr);
    ~FontPreviewWidget();

    void set_text(const QString& text, bool fallback_to_default = true);

public slots:
    void set_font(const QFont& font);

protected:
    void changeEvent ( QEvent* e ) override;
    void resizeEvent(QResizeEvent * event) override;
    void showEvent(QShowEvent * event) override;

private slots:
    void zoom_changed(double zoom);

private:
    class Private;
    std::unique_ptr<Private> d;
};

}

#endif // FONT_FONTPREVIEWWIDGET_H
