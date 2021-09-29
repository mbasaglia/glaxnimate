#include "custom_font.hpp"

#include <unordered_map>

#include <QFontDatabase>
#include <QCryptographicHash>

#include "app/utils/qbytearray_hash.hpp"

class glaxnimate::model::CustomFontDatabase::CustomFontData
{
public:
    CustomFontData() = default;

    CustomFontData(const QRawFont& font, int database_index, const QByteArray& data_hash, const QByteArray& data)
        : font(font),
        database_index(database_index),
        data_hash(data_hash),
        data(data)
    {}

    QString family_name() const
    {
        return font.familyName();
    }


    QRawFont font;
    int database_index = -1;
    QByteArray data_hash;
    QByteArray data;
    QString source_url;
    QString css_url;
    std::set<QString> name_aliases;
};

class glaxnimate::model::CustomFontDatabase::Private
{
public:
    std::unordered_map<int, DataPtr> fonts;
    // we keep track of hashes to avoid registering the exact same file twice
    std::unordered_map<QByteArray, int> hashes;

    std::unordered_map<QString, std::vector<int>> name_aliases;

    void tag_alias(const DataPtr& data, const QString& name)
    {
        if ( !name.isEmpty() && name != data->family_name() && data->name_aliases.insert(name).second )
            name_aliases[name].push_back(data->database_index);
    }

    void uninstall(std::unordered_map<int, DataPtr>::iterator iterator)
    {
        for ( const auto& name : iterator->second->name_aliases )
        {
            auto iter = name_aliases.find(name);
            if ( iter != name_aliases.end() )
            {
                if ( iter->second.size() <= 1 )
                    name_aliases.erase(iter);
                else
                    iter->second.erase(std::find(iter->second.begin(), iter->second.end(), iterator->second->database_index));
            }
        }

        hashes.erase(iterator->second->data_hash);
        QFontDatabase::removeApplicationFont(iterator->first);
        fonts.erase(iterator);
    }

    void remove_reference(int font)
    {
        auto it = fonts.find(font);
        if ( it == fonts.end() )
            return;

        if ( it->second.use_count() == 1 )
            uninstall(it);
    }

    DataPtr install(const QString& name_alias, const QByteArray& data)
    {
        auto hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
        auto hashit = hashes.find(hash);
        if ( hashit != hashes.end() )
        {
            auto item = fonts.at(hashit->second);
            tag_alias(item, name_alias);
            return item;
        }


        QRawFont raw(data, 16);
        if ( !raw.isValid() )
            return {};

        int index = QFontDatabase::addApplicationFontFromData(data);
        if ( index == -1 )
            return {};

        hashes[hash] = index;

        auto ptr = std::make_shared<CustomFontData>(raw, index, hash, data);
        fonts.emplace(index, ptr);
        tag_alias(ptr, name_alias);
        return ptr;
    }
};

glaxnimate::model::CustomFontDatabase::CustomFontDatabase()
    : d(std::make_unique<Private>())
{
}

glaxnimate::model::CustomFontDatabase::~CustomFontDatabase()
{
}

glaxnimate::model::CustomFontDatabase & glaxnimate::model::CustomFontDatabase::instance()
{
    static CustomFontDatabase instance;
    return instance;
}

std::vector<glaxnimate::model::CustomFont> glaxnimate::model::CustomFontDatabase::fonts() const
{
    std::vector<CustomFont> fonts;
    fonts.reserve(d->fonts.size());
    for ( const auto& font : d->fonts )
        fonts.emplace_back(font.second);
    return fonts;
}

glaxnimate::model::CustomFont glaxnimate::model::CustomFontDatabase::add_font(const QString& name_alias, const QByteArray& ttf_data)
{
    return d->install(name_alias, ttf_data);
}

glaxnimate::model::CustomFont glaxnimate::model::CustomFontDatabase::get_font(int database_index)
{
    auto it = d->fonts.find(database_index);
    if ( it == d->fonts.end() )
        return {};
    return it->second;
}

QFont glaxnimate::model::CustomFontDatabase::font(const QString& family, const QString& style_name, qreal size) const
{
    auto it = d->name_aliases.find(family);
    if ( it == d->name_aliases.end() )
    {
        QFont font(family);
        font.setPointSizeF(size);
        font.setStyleName(style_name);
        return font;
    }

    CustomFontData* match = d->fonts.at(it->second[0]).get();
    for ( int id : it->second )
    {
        const auto& font = d->fonts.at(id);
        if ( font->font.styleName() == style_name )
        {
            match = font.get();
            break;
        }
    }

    QFont font(match->family_name());
    font.setPointSizeF(size);
    font.setStyleName(style_name);
    return font;
}

std::unordered_map<QString, std::set<QString>> glaxnimate::model::CustomFontDatabase::aliases() const
{
    std::unordered_map<QString, std::set<QString>> map;

    for ( const auto& p : d->name_aliases )
    {
        std::set<QString> names;
        for ( const auto& id : p.second )
            names.insert(d->fonts.at(id)->family_name());
        map[p.first] = names;
    }

    return map;
}


glaxnimate::model::CustomFont::CustomFont(CustomFontDatabase::DataPtr dd)
    : d(std::move(dd))
{
    if ( !d )
        d = std::make_shared<CustomFontDatabase::CustomFontData>();
}

glaxnimate::model::CustomFont::CustomFont()
    : CustomFont(std::make_shared<CustomFontDatabase::CustomFontData>())
{
}

glaxnimate::model::CustomFont::CustomFont(int database_index)
    : CustomFont(CustomFontDatabase::instance().get_font(database_index))
{
}

glaxnimate::model::CustomFont::~CustomFont()
{
    if ( d )
    {
        int index = d->database_index;
        if ( index != -1 )
        {
            d = {};
            CustomFontDatabase::instance().d->remove_reference(index);
        }
    }
}

bool glaxnimate::model::CustomFont::is_valid() const
{
    return d->database_index != -1;
}

int glaxnimate::model::CustomFont::database_index() const
{
    return d->database_index;
}

QString glaxnimate::model::CustomFont::family() const
{
    return d->family_name();
}

QString glaxnimate::model::CustomFont::style_name() const
{
    return d->font.styleName();
}

QFont glaxnimate::model::CustomFont::font(int size) const
{
    QFont font(family(), size);
    font.setStyleName(style_name());
    return font;
}

const QRawFont & glaxnimate::model::CustomFont::raw_font() const
{
    return d->font;
}


QByteArray glaxnimate::model::CustomFont::data() const
{
    return d->data;
}

void glaxnimate::model::CustomFont::set_css_url(const QString& url)
{
    d->css_url = url;
}

void glaxnimate::model::CustomFont::set_source_url(const QString& url)
{
    d->source_url = url;
}

const QString & glaxnimate::model::CustomFont::css_url() const
{
    return d->css_url;
}

const QString & glaxnimate::model::CustomFont::source_url() const
{
    return d->source_url;
}
