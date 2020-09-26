#ifndef DOCUMENTSWATCHWIDGET_H
#define DOCUMENTSWATCHWIDGET_H

#include <QWidget>
#include <memory>


namespace model {

class Document;
class BrushStyle;
class NamedColor;
} // namespace model


class DocumentSwatchWidget : public QWidget
{
    Q_OBJECT
public:
    DocumentSwatchWidget(QWidget* parent = nullptr);
    ~DocumentSwatchWidget();

    void set_document(model::Document* document);

    void add_new_color(const QColor& color);

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void swatch_link(int index);
    void swatch_unlink();
    void swatch_add();
    void swatch_palette_color_added(int index);
    void swatch_palette_color_removed(int index);
    void swatch_palette_color_changed(int index);
    void swatch_doc_color_added(int position, model::NamedColor* color);
    void swatch_doc_color_removed(int pos);
    void swatch_doc_color_changed(int position, model::NamedColor* color);

signals:
    void current_color_def(model::BrushStyle* def);
    void needs_new_color();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // DOCUMENTSWATCHWIDGET_H
