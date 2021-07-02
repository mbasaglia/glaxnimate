#ifndef DOCUMENT_OPENER_HPP
#define DOCUMENT_OPENER_HPP


#include <QWidget>

#include "model/document.hpp"

namespace glaxnimate::android {


class DocumentOpener
{
public:
    DocumentOpener(QWidget* widget_parent);
    ~DocumentOpener();

    bool save(const QUrl& url, model::Document* document, io::Options& options) const;

    std::unique_ptr<model::Document> open(const QUrl& url) const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::android

#endif // DOCUMENT_OPENER_HPP
