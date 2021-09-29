#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QFontDatabase>

#include "model/document.hpp"

namespace glaxnimate::gui::font {

class FontModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum FontFilter {
        AllFonts = 0,
        ScalableFonts = 0x1,
        NonScalableFonts = 0x2,
        MonospacedFonts = 0x4,
        ProportionalFonts = 0x8
    };
    Q_DECLARE_FLAGS(FontFilters, FontFilter)
    Q_FLAG(FontFilters)

    explicit FontModel(QObject *parent = nullptr);
    ~FontModel();

    void set_document(model::Document* document);

    int columnCount(const QModelIndex & parent) const override;
    int rowCount(const QModelIndex & parent) const override;
    QVariant data(const QModelIndex & index, int role) const override;

    QStringList favourites() const;
    void set_favourites(const QStringList& faves);


    void set_writing_system(QFontDatabase::WritingSystem);
    QFontDatabase::WritingSystem writing_system() const;


    void set_font_filters(FontFilters filters);
    FontFilters font_filters() const;

    QModelIndex index_for_font(const QString& family);

    bool preview_font() const;
    void set_preview_font(bool preview);

    void set_favourite(const QString& family, bool favourite);
    void toggle_favourite(const QString& family);

private:
    void reset();

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::font
