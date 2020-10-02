#pragma once

#include <QPixmap>

#include "model/reference_target.hpp"

namespace model {

class Bitmap : public ObjectBase<Bitmap, ReferenceTarget>
{
    GLAXNIMATE_OBJECT
    GLAXNIMATE_PROPERTY(QByteArray, data, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY(QString, filename, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY_RO(QString, format, {})
    GLAXNIMATE_PROPERTY_RO(int, width, -1)
    GLAXNIMATE_PROPERTY_RO(int, height, -1)
    Q_PROPERTY(bool embedded READ embedded WRITE embed)

public:
    using Ctor::Ctor;

    void paint(QPainter* painter) const;

    bool embedded() const;

    QIcon reftarget_icon() const override;

    QString type_name_human() const override
    {
        return tr("Bitmap");
    }

    bool from_url(const QUrl& url);
    bool from_file(const QString& file);
    bool from_base64(const QString& data);

public slots:
    void refresh(bool rebuild_embedded);

    void embed(bool embedded);

private:
    void build_embedded(const QImage& img);

private slots:
    void on_refresh();

signals:
    void loaded();

private:
    QPixmap image;

};

} // namespace model
