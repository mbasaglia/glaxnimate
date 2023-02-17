#pragma once

#include <memory>
#include <functional>

#include <QDir>

#include "io/base.hpp"

namespace glaxnimate::io::avd {

class AvdParser
{
private:


public:
    /**
     * \throws SvgParseError on error
     */
    AvdParser(
        QIODevice* device,
        const QDir& resource_path,
        model::Document* document,
        const std::function<void(const QString&)>& on_warning = {},
        ImportExport* io = nullptr,
        QSize forced_size = {}
    );

    ~AvdParser();

    void parse_to_document();

    class Private;
private:
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::io::avd
