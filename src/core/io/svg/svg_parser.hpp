#pragma once

#include <memory>
#include <vector>
#include <exception>
#include <QIODevice>

namespace model {
    class Composition;
    class Document;
    class DocumentNode;
} // namespace model


namespace io::svg {

class SvgParseError : public std::exception
{
public:
    QString formatted(const QString& filename) const
    {
        return QString("%1:%2:%3: SVG Parse Error: %4")
            .arg(filename)
            .arg(line)
            .arg(column)
            .arg(message)
        ;
    }

    QString message;
    int line = -1;
    int column = -1;
};

class SvgParser
{
public:
    // How to parse <g> elements
    enum GroupMode
    {
        Groups,     ///< As group shapes
        Layers,     ///< As shape layers
        Inkscape,   ///< Follow inkscape:groupmode
    };

    /**
     * \throws SvgParseError on error
     */
    SvgParser(QIODevice* device,
            GroupMode group_mode,
            model::Document* document,
            model::Composition* composition);
    ~SvgParser();

    void parse_to_document(const std::function<void(const QString&)>& on_warning);
    std::vector<std::unique_ptr<model::DocumentNode>> parse_to_objects();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace io::svg
