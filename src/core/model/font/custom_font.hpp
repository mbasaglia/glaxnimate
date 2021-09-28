#pragma once


#include <memory>

#include <QRawFont>
#include <QObject>

namespace glaxnimate::model {

class CustomFont;

class CustomFontDatabase : public QObject
{
    Q_OBJECT

public:
    static CustomFontDatabase& instance();

    CustomFont add_font(const QByteArray& ttf_data);
    CustomFont get_font(int database_index);
    std::vector<CustomFont> fonts() const;

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
    explicit CustomFont(const QByteArray& data);
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

