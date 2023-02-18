#pragma once

#include <memory>
#include <functional>

#include <QDomDocument>
#include <QDomElement>


namespace glaxnimate::model {
    class Document;
} // namespace glaxnimate::model


namespace glaxnimate::io::avd {


class AvdRenderer
{
public:
    AvdRenderer(const std::function<void(const QString&)>& on_warning);
    ~AvdRenderer();

    void render(model::Document* document);

    QDomElement graphics();
    const std::vector<QDomElement>& animations();
    QDomDocument single_file();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::io::avd
