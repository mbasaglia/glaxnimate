#include "font_model.hpp"

#include <set>

#include <QIcon>

using namespace glaxnimate::gui;
using namespace glaxnimate;

class font::FontModel::Private
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

        for ( const auto& fam : database.families(system) )
        {
            if ( valid(fam) )
                fonts.push_back(fam);
        }
    }

    QFontDatabase database;
    QFontDatabase::WritingSystem system = QFontDatabase::Any;
    FontFilters filters = AllFonts;
    bool preview_font = true;

    std::set<QString> faves;
    QStringList filtered_faves;
    QStringList fonts;
};

font::FontModel::FontModel(QObject* parent)
    : QAbstractListModel(parent), d(std::make_unique<Private>())
{
    d->rebuild();
}

font::FontModel::~FontModel()
{
}


QVariant font::FontModel::data(const QModelIndex& index, int role) const
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

int font::FontModel::columnCount(const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    return 2;
}

QStringList font::FontModel::favourites() const
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

void font::FontModel::set_favourites(const QStringList& faves)
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

font::FontModel::FontFilters font::FontModel::font_filters() const
{
    return d->filters;
}

void font::FontModel::set_font_filters(font::FontModel::FontFilters filters)
{
    d->filters = filters;
    reset();
}

void font::FontModel::reset()
{
    beginResetModel();
    d->rebuild();
    endResetModel();
}

QModelIndex font::FontModel::index_for_font(const QString& family)
{
    int index = d->fonts.indexOf(family);
    if ( index == -1 )
        return {};
    return createIndex(index + d->filtered_faves.size(), 0, nullptr);
}

bool font::FontModel::preview_font() const
{
    return d->preview_font;
}

void font::FontModel::set_preview_font(bool preview)
{
    d->preview_font = preview;
    emit dataChanged(
        createIndex(0, 0, nullptr),
        createIndex(d->fonts.size() + d->filtered_faves.size(), 0, nullptr),
        {Qt::FontRole}
    );
}

QFontDatabase::WritingSystem font::FontModel::writing_system() const
{
    return d->system;
}

void font::FontModel::set_writing_system(QFontDatabase::WritingSystem system)
{
    d->system = system;
    reset();
}

int font::FontModel::rowCount(const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    return d->fonts.size() + d->filtered_faves.size();
}

void font::FontModel::set_favourite(const QString& family, bool favourite)
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

void font::FontModel::toggle_favourite(const QString& family)
{
    set_favourite(family, d->faves.count(family) == 0);
}
