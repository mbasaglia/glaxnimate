#ifndef FONTSTYLEWIDGET_H
#define FONTSTYLEWIDGET_H

#include <memory>
#include <QWidget>

#include "font_model.hpp"

namespace glaxnimate::gui::font {


class FontStyleWidget : public QWidget
{
    Q_OBJECT

public:
    FontStyleWidget(QWidget* parent = nullptr);
    ~FontStyleWidget();


    void set_font(const QFont& font);
    const QFont& font() const;

    FontModel& model();

protected:
    void changeEvent ( QEvent* e ) override;

signals:
    void font_edited(const QFont& font);
    void font_changed(const QFont& font);

private slots:
    void family_edited(const QString& family);
    void family_selected(const QModelIndex& index );
    void family_clicked(const QModelIndex& index);
    void style_selected(const QModelIndex& index);
    void size_edited(double size);
    void size_selected(const QModelIndex& index);
    void filter_flags_changed();
    void system_changed(int index);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::font

#endif // FONTSTYLEWIDGET_H
