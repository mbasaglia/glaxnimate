/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "font_model.hpp"

#include <set>

#include <QIcon>

#include "model/assets/assets.hpp"

class glaxnimate::gui::font::FontModel::Private
{
public:
    QString family(int index)
    {
        if ( index < 0 )
            return "";

        if ( index < filtered_faves.size() )
            return filtered_faves[index];

        index -= filtered_faves.size();
        if ( index < fonts.size() )
            return fonts[index];

        return "";
    }

    bool valid(const QString& family) const
    {
        constexpr const int scalable_mask = (ScalableFonts | NonScalableFonts);
        constexpr const int spacing_mask = (ProportionalFonts | MonospacedFonts);

        if ( (filters & scalable_mask) && (filters & scalable_mask) != scalable_mask )
            if ( bool(filters & ScalableFonts) != database.isSmoothlyScalable(family) )
                return false;

        if ( (filters & spacing_mask) && (filters & spacing_mask) != spacing_mask )
            if ( bool(filters & MonospacedFonts) != database.isFixedPitch(family) )
                return false;

        return true;
    }

    QStringList rebuild_faves()
    {
        QStringList filtered_faves;

        for ( const auto& fam : faves )
        {
            if ( valid(fam) )
                filtered_faves.push_back(fam);
        }

        return filtered_faves;
    }

    void rebuild()
    {
        filtered_faves = rebuild_faves();

        fonts.clear();

        std::set<QString> font_set;
        for ( const auto& fam : database.families(system) )
        {
            if ( valid(fam) )
                font_set.insert(fam);
        }

        if ( document )
        {
            for ( const auto& font : document->assets()->fonts->values )
                font_set.insert(font->family());

        }

        fonts.reserve(font_set.size());
        for ( const auto& family : font_set )
            fonts.push_back(family);
    }

    void maybe_remove_family(const QString& family, FontModel* parent)
    {
        if ( database.families(system).contains(family) )
            return;

        int row = fonts.indexOf(family);
        if ( row == -1 )
            return;

        parent->beginRemoveRows({}, row, row);

        fonts.erase(fonts.begin() + row);

        auto it = std::find(filtered_faves.begin(), filtered_faves.end(), family);
        if ( it != filtered_faves.end() )
            filtered_faves.erase(it);

        parent->endRemoveRows();
    }

    void maybe_add_family(const QString& family, FontModel* parent)
    {
        if ( fonts.contains(family) )
            return;

        auto iter = std::upper_bound(fonts.begin(), fonts.end(), family);
        int row = iter - fonts.begin();
        parent->beginInsertRows({}, row, row);
        fonts.insert(iter, family);
        parent->endInsertRows();
    }

    QFontDatabase database;
    QFontDatabase::WritingSystem system = QFontDatabase::Any;
    FontFilters filters = AllFonts;
    bool preview_font = true;

    std::set<QString> faves;
    QStringList filtered_faves;
    QStringList fonts;
    model::Document* document = nullptr;
};

glaxnimate::gui::font::FontModel::FontModel(QObject* parent)
    : QAbstractListModel(parent), d(std::make_unique<Private>())
{
    d->rebuild();
}

glaxnimate::gui::font::FontModel::~FontModel()
{
}

void glaxnimate::gui::font::FontModel::set_document(model::Document* document)
{
    if ( d->document == document )
        return;

    if ( d->document )
    {
        auto fonts = document->assets()->fonts.get();

        disconnect(fonts, nullptr, this, nullptr);

        for ( const auto& font : fonts->values )
            d->maybe_remove_family(font->family(), this);
    }

    d->document = document;

    if ( d->document )
    {
        auto fonts = document->assets()->fonts.get();

        for ( const auto& font : fonts->values )
            d->maybe_add_family(font->family(), this);


        connect(fonts, &model::FontList::font_added, this, [this](model::EmbeddedFont* font){
            if ( font->database_index() != -1 )
                d->maybe_add_family(font->family(), this);
        });

        connect(fonts, &model::DocumentNode::docnode_child_remove_end, this, [this](model::DocumentNode* node){
            auto font = static_cast<model::EmbeddedFont*>(node);
            d->maybe_remove_family(font->family(), this);
        });
    }
}


QVariant glaxnimate::gui::font::FontModel::data(const QModelIndex& index, int role) const
{
    QString family = d->family(index.row());
    if ( family.isEmpty() )
        return {};

    if ( index.column() == 1 )
    {
        if ( role == Qt::DecorationRole )
        {
            if ( d->faves.count(family) )
                return QIcon::fromTheme("starred-symbolic");
            else
                return QIcon::fromTheme("non-starred-symbolic");
        }
        return {};
    }

    switch ( role )
    {
        case Qt::ToolTipRole:
        case Qt::WhatsThisRole:
        case Qt::DisplayRole:
        case Qt::EditRole:
            return family;
        case Qt::FontRole:
//             if ( d->preview_font )
//                 return QFont(family);
            return {};
        case Qt::DecorationRole:
            if ( index.row() < d->filtered_faves.size() )
                return QIcon::fromTheme("favorite");
//             return QIcon::fromTheme("font-ttf");
    }

    return {};
}

int glaxnimate::gui::font::FontModel::columnCount(const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    return 2;
}

QStringList glaxnimate::gui::font::FontModel::favourites() const
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return QStringList(d->faves.begin(), d->faves.end());
#else
    QStringList l;
    for ( const auto& f : d->faves )
        l.push_back(f);
    return l;
#endif
}

void glaxnimate::gui::font::FontModel::set_favourites(const QStringList& faves)
{
    beginRemoveRows({}, 0, d->filtered_faves.size());
    d->faves.clear();
    d->faves.insert(faves.begin(), faves.end());
    d->filtered_faves.clear();
    endRemoveRows();
    auto new_faves = d->rebuild_faves();
    beginInsertRows({}, 0, new_faves.size());
    d->filtered_faves = new_faves;
    endInsertRows();
}

glaxnimate::gui::font::FontModel::FontFilters glaxnimate::gui::font::FontModel::font_filters() const
{
    return d->filters;
}

void glaxnimate::gui::font::FontModel::set_font_filters(glaxnimate::gui::font::FontModel::FontFilters filters)
{
    d->filters = filters;
    reset();
}

void glaxnimate::gui::font::FontModel::reset()
{
    beginResetModel();
    d->rebuild();
    endResetModel();
}

QModelIndex glaxnimate::gui::font::FontModel::index_for_font(const QString& family)
{
    int index = d->fonts.indexOf(family);
    if ( index == -1 )
        return {};
    return createIndex(index + d->filtered_faves.size(), 0, nullptr);
}

bool glaxnimate::gui::font::FontModel::preview_font() const
{
    return d->preview_font;
}

void glaxnimate::gui::font::FontModel::set_preview_font(bool preview)
{
    d->preview_font = preview;
    emit dataChanged(
        createIndex(0, 0, nullptr),
        createIndex(d->fonts.size() + d->filtered_faves.size(), 0, nullptr),
        {Qt::FontRole}
    );
}

QFontDatabase::WritingSystem glaxnimate::gui::font::FontModel::writing_system() const
{
    return d->system;
}

void glaxnimate::gui::font::FontModel::set_writing_system(QFontDatabase::WritingSystem system)
{
    d->system = system;
    reset();
}

int glaxnimate::gui::font::FontModel::rowCount(const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    return d->fonts.size() + d->filtered_faves.size();
}

void glaxnimate::gui::font::FontModel::set_favourite(const QString& family, bool favourite)
{
    if ( favourite == bool(d->faves.count(family)) )
        return;

    if ( favourite )
    {
        d->faves.insert(family);
        if ( d->valid(family) )
        {
            int index;
            for ( index = 0; index < d->filtered_faves.size(); index++ )
            {
                if ( d->filtered_faves[index] > family )
                    break;
            }

            beginInsertRows({}, index, index);
            d->filtered_faves.insert(index, family);
            endInsertRows();
        }
    }
    else
    {
        d->faves.erase(family);
        if ( d->valid(family) )
        {
            int index;
            for ( index = 0; index < d->filtered_faves.size(); index++ )
            {
                if ( d->filtered_faves[index] == family )
                    break;
            }

            beginRemoveRows({}, index, index);
            d->filtered_faves.removeAt(index);
            endRemoveRows();
        }
    }
}

void glaxnimate::gui::font::FontModel::toggle_favourite(const QString& family)
{
    set_favourite(family, d->faves.count(family) == 0);
}
