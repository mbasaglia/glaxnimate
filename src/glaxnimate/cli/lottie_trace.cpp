#include <memory>
#include <cstdio>

#include <QCoreApplication>
#include <QImage>
#include <QDebug>

#include "app/cli.hpp"

#include "glaxnimate/core/app_info.hpp"
#include "glaxnimate/core/model/document.hpp"
#include "glaxnimate/lottie/lottie_format.hpp"
#include "glaxnimate/raster/raster_mime.hpp"
#include "glaxnimate/trace/trace_wrapper.hpp"

struct File
{
    File(const QString& name)
        : name(name == "-" ? "/dev/stdout" : name)
    {}

    bool open(File* parent = nullptr)
    {
        if ( do_open(parent) )
            return true;

        qCritical() << "Could not open " << name;
        return false;
    }

    bool do_open(File* parent = nullptr)
    {
        if ( parent && parent->name == name )
        {
            file = parent->file;
            std_stream = parent->std_stream;
            return true;
        }

        if ( name.isEmpty() )
            return true;

        file = std::make_shared<QFile>();

        if ( name == "/dev/stdout" )
        {
            std_stream = true;
            return file->open(stdout, QIODevice::WriteOnly);
        }

        if ( name == "/dev/stderr" )
        {
            std_stream = true;
            return file->open(stderr, QIODevice::WriteOnly);
        }

        file->setFileName(name);
        return file->open(QIODevice::WriteOnly);
    }

    QString name;
    std::shared_ptr<QFile> file;
    bool std_stream = false;
};

int process(const app::cli::ParsedArguments& args)
{
    using namespace glaxnimate::trace;
    using namespace glaxnimate;

    File output_json(args.value("output").toString());

    if ( !output_json.open() )
        return 1;

    File output_rendered(args.value("rendered").toString());
    if ( !output_rendered.open(&output_json) )
        return 1;

    QString image_filename = args.value("image").toString();
    QImage image(image_filename);
    if ( image.isNull() )
    {
        qCritical() << "Could not open input image";
        return 1;
    }


    SegmentedImage segmented = segment(image);
    model::Document document(image_filename);
    trace::TraceWrapper trace(&document, image, image_filename);
    std::vector<trace::TraceWrapper::TraceResult> result;
    auto preset = trace.preset_suggestion();
    trace.trace_preset(preset, 256, result);
    trace.apply(result, preset == trace::TraceWrapper::PixelPreset ? 0 : 1);

    if ( output_json.file )
        output_json.file->write(io::lottie::LottieFormat().save(&document, {}, output_json.name));

    QByteArray separator = ('\n' + args.value("separator").toString() + '\n').toUtf8();

    if ( output_rendered.file )
    {
        if ( output_rendered.file == output_json.file )
            output_rendered.file->write(separator);

        QImage rendered = io::raster::RasterMime::to_image({document.main()});
        rendered.save(output_rendered.file.get(), output_rendered.std_stream ? "PNG" : nullptr);
    }

    return 0;
}


int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    auto& info = glaxnimate::AppInfo::instance();
    application.setApplicationName("lottie_trace");
    application.setApplicationVersion(info.version());
    application.setOrganizationName(info.organization());

    app::cli::Parser parser("Trace raster to lottie");
    parser.add_group(QCoreApplication::tr("Informational Options"));
    parser.add_argument({{"--help", "-h"}, QCoreApplication::tr("Show this help and exit"), app::cli::Argument::ShowHelp});
    parser.add_argument({{"--version", "-v"}, QCoreApplication::tr("Show version information and exit"), app::cli::Argument::String});

    parser.add_group(QCoreApplication::tr("Options"));
    parser.add_argument({{"image"}, QCoreApplication::tr("Image file to trace")});
    parser.add_argument({{"--output", "-o"}, QCoreApplication::tr("Output JSON lottie file"), app::cli::Argument::String, "-"});
    parser.add_argument({{"--rendered", "-r"}, QCoreApplication::tr("Output rendered PNG file"), app::cli::Argument::String});
    parser.add_argument({{"--separator", "-s"}, QCoreApplication::tr("Separator to use when -o and -r have the same value"), app::cli::Argument::String, "==="});

    return process(parser.parse(application.arguments()));
}

