#pragma once

#include <QRawFont>

#include "asset.hpp"

namespace glaxnimate::model {

class CustomFont;

class CustomFontDatabase : public QObject
{
    Q_OBJECT

public:
    static CustomFontDatabase& instance();

    CustomFont add_font(const QByteArray& ttf_data);
    CustomFont get_font(int database_index);

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
    QByteArray data() const;

private:
    CustomFontDatabase::DataPtr d;
};


class EmbeddedFont : public Asset
{
    GLAXNIMATE_OBJECT(EmbeddedFont)

    GLAXNIMATE_PROPERTY(QByteArray, data, {}, &EmbeddedFont::on_data_changed)
    GLAXNIMATE_PROPERTY(QString, source_url, {})
    GLAXNIMATE_PROPERTY(QString, css_url, {})

    Q_PROPERTY(QString family READ family)
    Q_PROPERTY(QString style_name READ style_name)
    Q_PROPERTY(int database_index READ database_index)

public:
    EmbeddedFont(model::Document* document);
    EmbeddedFont(model::Document* document, const QByteArray& data, const QString& source_url, const QString& css_url, CustomFont custom_font);

    QIcon instance_icon() const override;
    QString type_name_human() const override;
    QString object_name() const override;
    bool remove_if_unused(bool clean_lists) override;


    QString family() const { return custom_font_.family(); }
    QString style_name() const { return custom_font_.style_name(); }
    int database_index() const { return custom_font_.database_index(); }

private:
    void on_data_changed();

    CustomFont custom_font_;
};

} // namespace glaxnimate::model
