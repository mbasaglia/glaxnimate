#ifndef GLAXNIMATE_GUI_FONT_GOOGLEFONTSWIDGET_H
#define GLAXNIMATE_GUI_FONT_GOOGLEFONTSWIDGET_H

#include <memory>
#include <QWidget>

#include "google_fonts_model.hpp"

namespace glaxnimate::gui::font {

class GoogleFontsWidget : public QWidget
{
    Q_OBJECT

public:
    GoogleFontsWidget(QWidget* parent = nullptr);
    ~GoogleFontsWidget();

    const GoogleFontsModel& model() const;

protected:
    void changeEvent ( QEvent* e ) override;
    void showEvent(QShowEvent* e) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::font

#endif // GLAXNIMATE_GUI_FONT_GOOGLEFONTSWIDGET_H
