#pragma once

#include "io_registry.hpp"

class QJsonDocument;
class QJsonObject;

namespace glaxnimate::io {

class JsonImporter : public ImportExport
{
    Q_OBJECT

public:
    QStringList extensions() const override { return {"json"}; }
    bool can_open() const override { return true; }
    bool ignore_extension() const override { return true; }

    bool load_json_data(const QByteArray& data, model::Document* document, const QVariantMap& settings = {}, const QString& filename = {});
    bool load_json(const QJsonDocument& jdoc, model::Document* document, const QVariantMap& settings = {}, const QString& filename = {});


protected:
    bool on_open(QIODevice& file, const QString& filename, model::Document* document, const QVariantMap& settings) override;

    virtual bool on_load_json_object(const QJsonObject& json, model::Document* document, const QVariantMap& settings, const QString& filename) = 0;
    virtual bool can_load_object(const QJsonObject& json) = 0;

    static std::vector<JsonImporter*>& registered_importers();

    template<class Derived>
    class Autoreg : public io::Autoreg<Derived>
    {
    public:
        template<class... Args>
        Autoreg(Args&&... args) : io::Autoreg<Derived>(std::forward<Args>(args)...)
        {
            registered_importers().push_back(this->registered);
        }
    };

    friend class JsonDispatchImporter;
};


class JsonDispatchImporter : public JsonImporter
{
public:
    QString slug() const override { return "lottie"; }
    QString name() const override { return tr("JSON - Autodetect Format"); }
    bool can_save() const override { return false; }
    bool ignore_extension() const override { return false; }

protected:
    bool can_load_object(const QJsonObject& jdoc) override;
    bool on_load_json_object(const QJsonObject& jdoc, model::Document* document, const QVariantMap& settings, const QString& filename) override;
private:
    static io::Autoreg<JsonDispatchImporter> autoreg;
};

} // namespace glaxnimate::io
