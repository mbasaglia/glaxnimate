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
        : name(name)
    {}

    bool open(QIODevice::OpenModeFlag flag, File* parent = nullptr)
    {
        if ( do_open(flag, parent) )
            return true;

        qCritical() << "Could not open " << name;
        return false;
    }

    bool do_open(QIODevice::OpenModeFlag flags, File* parent = nullptr)
    {
        if ( name == "-" )
            name = flags & QIODevice::WriteOnly ? "/dev/stdout" : "/dev/stdin";

        if ( parent && parent->name == name )
        {
            file = parent->file;
            std_stream = parent->std_stream;
            return true;
        }

        if ( name.isEmpty() || name == "/dev/null" )
            return true;

        file = std::make_shared<QFile>();

        if ( name == "/dev/stdout" )
        {
            std_stream = true;
            return file->open(stdout, flags);
        }

        if ( name == "/dev/stderr" )
        {
            std_stream = true;
            return file->open(stderr, flags);
        }

        if ( name == "/dev/stdin" )
        {
            std_stream = true;
            return file->open(stdin, flags);
        }

        file->setFileName(name);
        return file->open(flags);
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

    if ( !output_json.open(QIODevice::WriteOnly) )
        return 1;

    File output_rendered(args.value("rendered").toString());
    if ( !output_rendered.open(QIODevice::WriteOnly, &output_json) )
        return 1;

    QString image_filename = args.value("image").toString();
    QImage image(image_filename);
    if ( image.isNull() )
    {
        app::cli::show_message("Could not open input image", true);
        return 1;
    }

    model::Document document(image_filename);
    QString preset_arg = args.get<QString>("preset");
    int posterization = args.get<int>("color-count");
    std::vector<trace::TraceWrapper::TraceResult> result;
    trace::TraceWrapper trace(&document, image, image_filename);
    trace.options().set_min_area(args.get<int>("min-area"));
    trace.options().set_smoothness(args.get<qreal>("smoothness"));
    if ( preset_arg == "manual" )
    {
        auto algo = args.get<QString>("palette-algorithm");
        BrushData brushes;

        if ( algo == "cluster-merge" )
        {
            brushes = cluster_merge(trace.segmented_image(), posterization, args.get<int>("cluster-merge-min-area"), args.get<int>("cluster-merge-min-distance"));
        }
        else
        {
            if ( algo == "k-modes" )
            {
                brushes.colors = k_modes(trace.segmented_image().histogram(), posterization);
            }
            else if ( algo == "k-means" )
            {
                KMeansMatch match = KMeansMatch::None;
                auto match_arg = args.get<QString>("k-means-match");
                if ( match_arg == "None" )
                    match = KMeansMatch::None;
                else if ( match_arg == "Closest" )
                    match = KMeansMatch::Closest;
                else if ( match_arg == "MostFrequent" )
                    match = KMeansMatch::MostFrequent;
                brushes.colors = k_means(trace.segmented_image().histogram(), posterization, args.get<int>("k-means-iterations"), match);
            }
            else if ( algo == "eem" )
            {
                brushes.colors = edge_exclusion_modes(trace.segmented_image(), posterization, args.get<int>("eem-min-area"));
            }
            else
            {
                app::cli::show_message(QObject::tr("%1 is not a valid algorithm").arg(algo) , true);
                return 1;
            }

            trace.segmented_image().quantize(brushes.colors);
        }

        trace.trace_closest(brushes, result);
        trace.apply(result, args.get<qreal>("stroke"));
    }
    else
    {
        glaxnimate::trace::TraceWrapper::Preset preset;
        if ( preset_arg == "auto" )
            preset = trace.preset_suggestion();
        else if ( preset_arg == "pixel" )
            preset = glaxnimate::trace::TraceWrapper::PixelPreset;
        else if ( preset_arg == "flat" )
            preset = glaxnimate::trace::TraceWrapper::FlatPreset;
        else if ( preset_arg == "complex" )
            preset = glaxnimate::trace::TraceWrapper::ComplexPreset;
        else
            app::cli::show_message(QObject::tr("%1 is not a valid preset").arg(preset_arg) , true);

        trace.trace_preset(preset, posterization, result);
        trace.apply(result, preset == trace::TraceWrapper::PixelPreset ? 0 : 1);
    }

    document.main()->width.set(image.width());
    document.main()->height.set(image.height());

    if ( output_json.file )
        output_json.file->write(io::lottie::LottieFormat().save(&document, {}, output_json.name));

    QByteArray separator(1, 0);

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

    parser.add_group(QCoreApplication::tr("File Options"));
    parser.add_argument({{"image"}, QCoreApplication::tr("Image file to trace")});
    parser.add_argument({{"--output", "-o"}, QCoreApplication::tr("Output JSON lottie file"), app::cli::Argument::String, "-"});
    parser.add_argument({{"--rendered", "-r"}, QCoreApplication::tr("Output rendered PNG file"), app::cli::Argument::String});

    parser.add_group(QCoreApplication::tr("Trace Options"));
    parser.add_argument({{"--preset"}, QCoreApplication::tr("Preset to use"),
                        app::cli::Argument::String, "auto", {}, {}, 1, {"auto", "manual", "flat", "pixel", "complex"}});
    parser.add_argument({{"--color-count"}, QCoreApplication::tr("Number of colors / posterization level"), app::cli::Argument::Int, 32});

    parser.add_group(QCoreApplication::tr("Segmentation Options"));
    parser.add_argument({{"--palette-algorithm"}, QCoreApplication::tr("Algorithm to find the palette"),
                        app::cli::Argument::String, "cluster_merge", {}, {}, 1, {"k-modes", "k-means", "octree", "eem", "cluster-merge"}});

    parser.add_argument({{"--k-means-match"}, "", app::cli::Argument::String, "MostFrequent", {}, {}, 1, {"None", "MostFrequent", "Closest"}});
    parser.add_argument({{"--k-means-iterations"}, "", app::cli::Argument::Int, 100});
    parser.add_argument({{"--eem-min-area"}, "", app::cli::Argument::Int, 4});
    parser.add_argument({{"--cluster-merge-min-area"}, "", app::cli::Argument::Int, 4});
    parser.add_argument({{"--cluster-merge-min-distance"}, "", app::cli::Argument::Int, 16});

    parser.add_group(QCoreApplication::tr("Trace Output Options"));
    parser.add_argument({{"--smoothness"}, "", app::cli::Argument::Float, 0.75});
    parser.add_argument({{"--min-area"}, "", app::cli::Argument::Int, 4});
    parser.add_argument({{"--stroke"}, "", app::cli::Argument::Float, 1});

    auto args = parser.parse(application.arguments());
    if ( args.return_value )
        return *args.return_value;
    return process(args);
}

