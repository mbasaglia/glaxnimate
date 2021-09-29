#pragma once

#include <set>
#include <memory>
#include <unordered_map>

#include <QRawFont>
#include <QObject>

#include "app/utils/qstring_hash.hpp"

namespace glaxnimate::model {

class CustomFont;

enum class FontFileFormat
{
    Unknown,
    TrueType,
    OpenType,
    Woff2,
    Woff
};


class CustomFontDatabase : public QObject
{
    Q_OBJECT

public:
    static CustomFontDatabase& instance();
    static FontFileFormat font_data_format(const QByteArray& data);

    CustomFont add_font(const QString& name_alias, const QByteArray& ttf_data);
    CustomFont get_font(int database_index);
    std::vector<CustomFont> fonts() const;

    QFont font(const QString& family, const QString& style_name, qreal size) const;
    std::unordered_map<QString, std::set<QString>> aliases() const;

private:
    CustomFontDatabase();
    ~CustomFontDatabase();
    CustomFontDatabase(const CustomFontDatabase&) = delete;
    CustomFontDatabase& operator=(const CustomFontDatabase&) = delete;

    class Private;
    class CustomFontData;
    std::unique_ptr<Private> d;
    using DataPtr = std::shared_ptr<CustomFontData>;
    friend CustomFont;
};

class CustomFont
{
public:
    explicit CustomFont(int database_index);
    CustomFont(CustomFontDatabase::DataPtr d);
    CustomFont();
    ~CustomFont();

    bool is_valid() const;
    QString family() const;
    QString style_name() const;
    int database_index() const;
    QFont font(int size) const;
    const QRawFont& raw_font() const;
    QByteArray data() const;

    const QString& source_url() const;
    const QString& css_url() const;

    void set_source_url(const QString& url);
    void set_css_url(const QString& url);

private:
    CustomFontDatabase::DataPtr d;
};

} // namespace glaxnimate::model

