#include "embedded_font.hpp"

#include <QFontDatabase>

#include "model/document.hpp"
#include "command/object_list_commands.hpp"
#include "assets.hpp"

class glaxnimate::model::CustomFontDatabase::CustomFontData
{
public:
    CustomFontData() = default;

    CustomFontData(const QRawFont& font, int database_index)
        : font(font),
        database_index(database_index)
    {}


    QRawFont font;
    int database_index = -1;
};

class glaxnimate::model::CustomFontDatabase::Private
{
public:
    std::unordered_map<int, DataPtr> fonts;

    void uninstall(std::unordered_map<int, DataPtr>::iterator iterator)
    {
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
        QRawFont raw(data, 16);
        if ( !raw.isValid() )
            return {};

        int index = QFontDatabase::addApplicationFont(data);
        if ( index == -1 )
            return {};

        auto it = fonts.find(index);
        if ( it != fonts.end() )
            return it->second;

        auto ptr = std::make_shared<CustomFontData>(raw, index);
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

glaxnimate::model::CustomFont::CustomFont(CustomFontDatabase::DataPtr d)
    : d(std::move(d))
{
    if ( !d )
        d = std::make_shared<CustomFontDatabase::CustomFontData>();
}

glaxnimate::model::CustomFont::CustomFont()
    : CustomFont(std::make_shared<CustomFontDatabase::CustomFontData>())
{
}

glaxnimate::model::CustomFont::CustomFont(int database_index)
    : CustomFont(std::move(CustomFontDatabase::instance().get_font(database_index)))
{
}

glaxnimate::model::CustomFont::CustomFont(const QByteArray& data)
    : CustomFont(CustomFontDatabase::instance().d->install(data))
{
}

glaxnimate::model::CustomFont::~CustomFont()
{
    int index = d->database_index;
    if ( index != -1 )
    {
        d = {};
        CustomFontDatabase::instance().d->remove_reference(index);
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

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::EmbeddedFont)


glaxnimate::model::EmbeddedFont::EmbeddedFont(model::Document* document)
    : Asset(document)
{
}

glaxnimate::model::EmbeddedFont::EmbeddedFont(model::Document* document, const QByteArray& data, const QString& source_url, const QString& css_url, CustomFont custom_font)
    : Asset(document), custom_font_(std::move(custom_font))
{
    this->data.set(data);
    this->source_url.set(source_url);
    this->css_url.set(css_url);
}


QIcon glaxnimate::model::EmbeddedFont::instance_icon() const
{
    return QIcon::fromTheme("font");
}

QString glaxnimate::model::EmbeddedFont::object_name() const
{
    return custom_font_.family() + " " + custom_font_.style_name();
}

QString glaxnimate::model::EmbeddedFont::type_name_human() const
{
    return tr("Font");
}

bool glaxnimate::model::EmbeddedFont::remove_if_unused(bool)
{
    if ( users().empty() )
    {
        document()->push_command(new command::RemoveObject(
            this,
            &document()->assets()->fonts->values
        ));
        return true;
    }
    return false;
}

void glaxnimate::model::EmbeddedFont::on_data_changed()
{
    custom_font_ = CustomFontDatabase::instance().add_font(data.get());
}
