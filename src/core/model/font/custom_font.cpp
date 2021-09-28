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


    QRawFont font;
    int database_index = -1;
    QByteArray data_hash;
    QByteArray data;
    QString source_url;
    QString css_url;
};

class glaxnimate::model::CustomFontDatabase::Private
{
public:
    std::unordered_map<int, DataPtr> fonts;
    // we keep track of hashes to avoid registering the exact same file twice
    std::unordered_map<QByteArray, int> hashes;

    void uninstall(std::unordered_map<int, DataPtr>::iterator iterator)
    {
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

    DataPtr install(const QByteArray& data)
    {
        auto hash = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
        auto hashit = hashes.find(hash);
        if ( hashit != hashes.end() )
            return fonts.at(hashit->second);


        QRawFont raw(data, 16);
        if ( !raw.isValid() )
            return {};

        int index = QFontDatabase::addApplicationFontFromData(data);
        if ( index == -1 )
            return {};

        hashes[hash] = index;

        auto ptr = std::make_shared<CustomFontData>(raw, index, hash, data);
        fonts.emplace(index, ptr);
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

glaxnimate::model::CustomFont glaxnimate::model::CustomFontDatabase::add_font(const QByteArray& ttf_data)
{
    return d->install(ttf_data);
}

glaxnimate::model::CustomFont glaxnimate::model::CustomFontDatabase::get_font(int database_index)
{
    auto it = d->fonts.find(database_index);
    if ( it == d->fonts.end() )
        return {};
    return it->second;
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

glaxnimate::model::CustomFont::CustomFont(const QByteArray& data)
    : CustomFont(CustomFontDatabase::instance().d->install(data))
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
    return d->font.familyName();
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
