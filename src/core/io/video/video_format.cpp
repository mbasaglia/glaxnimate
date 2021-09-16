#include "video_format.hpp"

#include <mutex>
#include <cstring>
#include <set>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
}

#include "app/qstring_exception.hpp"
#include "app/log/log.hpp"

namespace glaxnimate::av {

template<int max_size, class Callback, class... Args>
QString to_str(Callback callback, Args... args)
{
    char buf[max_size] = {0};
    return callback(buf, args...);
}

QString err2str(int errnum)
{
    return to_str<AV_ERROR_MAX_STRING_SIZE>(&av_make_error_string, AV_ERROR_MAX_STRING_SIZE, errnum);
}

class Error: public app::QStringException<>{ using Ctor::Ctor; };


template<class Callback, class Object>
class CGuard
{
public:
    CGuard(Callback callback, Object object)
    : callback(callback), object(object) {}

    ~CGuard()
    {
        callback(object);
    }

private:
    Callback callback;
    Object object;
};


// a wrapper around a single output AVStream
struct OutputStream
{
    OutputStream(
        AVFormatContext *oc,
        AVCodecID codec_id
    )
    {
        format_context = oc;

        // find the encoder
        codec = (AVCodec*)avcodec_find_encoder(codec_id);
        if ( !codec )
            throw av::Error(QObject::tr("Could not find encoder for '%1'").arg(avcodec_get_name(codec_id)));

        stream = avformat_new_stream(oc, nullptr);
        if (!stream)
            throw av::Error(QObject::tr("Could not allocate stream"));

        stream->id = oc->nb_streams-1;
        codec_context = avcodec_alloc_context3(codec);
        if ( !codec_context )
            throw av::Error(QObject::tr("Could not alloc an encoding context"));

        // Some formats want stream headers to be separate.
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    ~OutputStream()
    {
        avcodec_free_context(&codec_context);
        av_frame_free(&frame);
        av_frame_free(&tmp_frame);
        sws_freeContext(sws_context);
    }

    int read_packets()
    {
        int ret = 0;

        while ( ret >= 0 )
        {
            AVPacket pkt;
            memset(&pkt, 0, sizeof(AVPacket));

            ret = avcodec_receive_packet(codec_context, &pkt);
            if ( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                break;
            else if (ret < 0)
                throw av::Error(QObject::tr("Error encoding a frame: %1").arg(av::err2str(ret)));

            // rescale output packet timestamp values from codec to stream timebase
            av_packet_rescale_ts(&pkt, codec_context->time_base, stream->time_base);
            pkt.stream_index = stream->index;

            // Write the compressed frame to the media file.
            ret = av_interleaved_write_frame(format_context, &pkt);
            av_packet_unref(&pkt);
            if (ret < 0)
                throw av::Error(QObject::tr("Error while writing output packet: %1").arg(av::err2str(ret)));
        }

        return ret;
    }

    int write_frame(AVFrame *frame)
    {
        // send the frame to the encoder
        int ret = avcodec_send_frame(codec_context, frame);
        if ( ret < 0 )
            throw av::Error(QObject::tr("Error sending a frame to the encoder: %1").arg(av::err2str(ret)));

        return read_packets();
    }


    int flush_frames()
    {
        return write_frame(nullptr);
    }

    AVStream *stream = nullptr;
    AVCodecContext *codec_context = nullptr;

    // pts of the next frame that will be generated
    int64_t next_pts = 0;

    AVFrame *frame = nullptr;
    AVFrame *tmp_frame = nullptr;

    SwsContext *sws_context = nullptr;
    AVFormatContext *format_context = nullptr;
    AVCodec *codec = nullptr;
};

class DictWrapper
{
public:
    class Item
    {
    public:
        Item(AVDictionary** av_dict, QByteArray key)
        :  av_dict(av_dict), key(std::move(key)), value(nullptr)
        {}

        operator const char*() const
        {
            return get();
        }

        const char* get() const
        {
            if ( value == nullptr )
            {
                if ( auto entry = av_dict_get(*av_dict, key.data(), nullptr, 0) )
                    value = entry->value;
            }
            return value;
        }

        void set(const char* text)
        {
            int ret = av_dict_set(av_dict, key.data(), text, 0);
            if ( ret >= 0 )
                value = nullptr;
            else
                throw Error(QObject::tr("Could not set dict key `%1`: %2").arg(QString(key)).arg(err2str(ret)));
        }

        void set(const QString& s)
        {
            set(s.toUtf8().data());
        }

        void set(int64_t v)
        {
            int ret = av_dict_set_int(av_dict, key.data(), v, 0);
            if ( ret >= 0 )
                value = nullptr;
            else
                throw Error(QObject::tr("Could not set dict key `%1`: %2").arg(QString(key)).arg(err2str(ret)));
        }

        Item& operator=(const char* text)
        {
            set(text);
            return *this;
        }

        Item& operator=(const QString& text)
        {
            set(text);
            return *this;
        }

        Item& operator=(int64_t v)
        {
            set(v);
            return *this;
        }

    private:
        AVDictionary** av_dict;
        QByteArray key;
        mutable const char* value;
    };

    DictWrapper(AVDictionary** av_dict)
    : av_dict(av_dict)
    {}

    Item operator[](const QString& key)
    {
        return Item(av_dict, key.toUtf8());
    }

    int size() const
    {
        return av_dict_count(*av_dict);
    }

    void erase(const QString& key)
    {
        int ret = av_dict_set(av_dict, key.toUtf8().data(), nullptr, 0);
        if ( ret < 0 )
            throw Error(QObject::tr("Could not erase dict key `%1`: %2").arg(key).arg(err2str(ret)));
    }

private:
    AVDictionary** av_dict;
};

class Dict : public DictWrapper
{
public:
    Dict() : DictWrapper(&local_dict) {}

    Dict(Dict&& other) : Dict()
    {
        std::swap(local_dict, other.local_dict);
    }

    Dict(const Dict& other) : Dict()
    {
        int ret = av_dict_copy(&local_dict, other.local_dict, 0);
        if ( ret < 0 )
            throw Error(QObject::tr("Could not copy dict: %1").arg(err2str(ret)));
    }

    Dict& operator=(Dict&& other)
    {
        std::swap(local_dict, other.local_dict);
        return *this;
    }

    Dict& operator=(const Dict& other)
    {
        int ret = av_dict_copy(&local_dict, other.local_dict, 0);
        if ( ret < 0 )
            throw Error(QObject::tr("Could not copy dict `%1`: %2").arg(err2str(ret)));
        return *this;
    }

    ~Dict()
    {
        av_dict_free(&local_dict);
    }

    AVDictionary** dict()
    {
        return &local_dict;
    }

private:
    AVDictionary* local_dict = nullptr;
};

class Video
{
public:
    static AVFrame *alloc_picture(AVPixelFormat pix_fmt, int width, int height)
    {
        AVFrame *picture;
        int ret;

        picture = av_frame_alloc();
        if (!picture)
            return nullptr;

        picture->format = pix_fmt;
        picture->width  = width;
        picture->height = height;

        // allocate the buffers for the frame data
        ret = av_frame_get_buffer(picture, 0);
        if (ret < 0)
            throw av::Error(QObject::tr("Could not allocate frame data."));

        return picture;
    }

    static std::pair<AVPixelFormat, QImage::Format> image_format(QImage::Format format)
    {
        switch ( format )
        {
            case QImage::Format_Invalid:
            default:
                return {AV_PIX_FMT_NONE, QImage::Format_Invalid};
            case QImage::Format_Mono:
            case QImage::Format_MonoLSB:
                return {AV_PIX_FMT_MONOBLACK, format};
            case QImage::Format_Indexed8:
                return {AV_PIX_FMT_ARGB, QImage::Format_RGB32};
            case QImage::Format_RGB32:
                return {AV_PIX_FMT_0RGB, format};
            case QImage::Format_ARGB32:
                return {AV_PIX_FMT_ARGB, format};
            case QImage::Format_ARGB32_Premultiplied:
                return {AV_PIX_FMT_ARGB, format};
            case QImage::Format_RGB16:
                return {AV_PIX_FMT_RGB565LE, format};
            case QImage::Format_RGB555:
                return {AV_PIX_FMT_RGB555LE, format};
            case QImage::Format_RGB888:
                return {AV_PIX_FMT_RGB24, format};
            case QImage::Format_RGBX8888:
                return {AV_PIX_FMT_RGB0, format};
            case QImage::Format_RGBA8888:
            case QImage::Format_RGBA8888_Premultiplied:
                return {AV_PIX_FMT_RGBA, format};
            case QImage::Format_Alpha8:
            case QImage::Format_Grayscale8:
                return {AV_PIX_FMT_GRAY8, format};
            case QImage::Format_RGBA64_Premultiplied:
            case QImage::Format_ARGB8555_Premultiplied:
            case QImage::Format_ARGB8565_Premultiplied:
            case QImage::Format_ARGB6666_Premultiplied:
            case QImage::Format_ARGB4444_Premultiplied:
            case QImage::Format_A2RGB30_Premultiplied:
            case QImage::Format_A2BGR30_Premultiplied:
                return {AV_PIX_FMT_ARGB, QImage::Format_ARGB32};
            case QImage::Format_RGB30:
            case QImage::Format_RGB444:
            case QImage::Format_RGB666:
            case QImage::Format_BGR30:
            case QImage::Format_RGBX64:
            case QImage::Format_RGBA64:
                return {AV_PIX_FMT_RGB24, QImage::Format_RGB888};
        }
    }

    Video(AVFormatContext *oc, Dict options, int64_t bit_rate, int width, int height, int fps)
        : ost(oc, oc->oformat->video_codec)
    {
        if ( ost.codec->type != AVMEDIA_TYPE_VIDEO )
            throw Error(QObject::tr("No video codec"));

        ost.codec_context->codec_id = oc->oformat->video_codec;

        ost.codec_context->bit_rate = bit_rate;

        // Resolution must be a multiple of two
        ost.codec_context->width = width;
        if ( ost.codec_context->width % 2 )
            ost.codec_context->width -= 1;
        ost.codec_context->height = height;
        if ( ost.codec_context->height % 2 )
            ost.codec_context->height -= 1;

        // timebase: This is the fundamental unit of time (in seconds) in terms
        // of which frame timestamps are represented. For fixed-fps content,
        // timebase should be 1/framerate and timestamp increments should be
        // identical to 1.
        ost.stream->time_base = AVRational{ 1, fps };
        ost.codec_context->time_base = ost.stream->time_base;

        // emit one intra frame every twelve frames at most
        ost.codec_context->gop_size = 12;
        if ( ost.codec->pix_fmts == nullptr )
            throw av::Error(QObject::tr("Could not determine pixel format"));
        // get_format() for some reason returns an invalid value
        ost.codec_context->pix_fmt = ost.codec->pix_fmts[0];

        // just for testing, we also add B-frames
        if ( ost.codec_context->codec_id == AV_CODEC_ID_MPEG2VIDEO )
            ost.codec_context->max_b_frames = 2;

        int ret;
        // open the codec
        ret = avcodec_open2(ost.codec_context, ost.codec, options.dict());
        if (ret < 0)
            throw av::Error(QObject::tr("Could not open video codec: %1").arg(av::err2str(ret)));

        // allocate and init a re-usable frame
        ost.frame = alloc_picture(ost.codec_context->pix_fmt, ost.codec_context->width, ost.codec_context->height);
        if (!ost.frame)
            throw av::Error(QObject::tr("Could not allocate video frame"));

        ost.tmp_frame = nullptr;

        /* copy the stream parameters to the muxer */
        ret = avcodec_parameters_from_context(ost.stream->codecpar, ost.codec_context);
        if (ret < 0)
            throw av::Error(QObject::tr("Could not copy the stream parameters"));
    }

    static void fill_image(AVFrame *pict, const QImage& image)
    {
        for ( int y = 0; y < image.height(); y++)
        {
            auto line = image.constScanLine(y);
            for ( int x = 0; x < image.bytesPerLine(); x++ )
            {
                pict->data[0][y * pict->linesize[0] + x] = line[x];
            }
        }
    }

    AVFrame *get_video_frame(QImage image)
    {
        // when we pass a frame to the encoder, it may keep a reference to it
        // internally; make sure we do not overwrite it here
        if ( av_frame_make_writable(ost.frame) < 0 )
            throw av::Error(QObject::tr("Error while creating video frame"));

        auto format = image_format(image.format());
        if ( format.first == AV_PIX_FMT_NONE )
        {
            image = QImage(ost.codec_context->width, ost.codec_context->height, QImage::Format_RGB888);
            format.first = AV_PIX_FMT_RGB24;
        }
        else if ( format.second != image.format() )
        {
            image = image.convertToFormat(format.second);
        }

        if ( ost.codec_context->pix_fmt != format.first || image.width() != ost.codec_context->width || image.height() != ost.codec_context->height )
        {
            if (!ost.sws_context)
            {
                ost.sws_context = sws_getContext(
                    image.width(), image.height(), format.first,
                    ost.codec_context->width, ost.codec_context->height, ost.codec_context->pix_fmt,
                    SWS_BICUBIC,
                    nullptr, nullptr, nullptr
                );
                if (!ost.sws_context)
                    throw av::Error(QObject::tr("Could not initialize the conversion context"));
            }
            if ( !ost.tmp_frame )
            {
                ost.tmp_frame = alloc_picture(format.first, image.width(), image.height());
                if (!ost.tmp_frame)
                    throw av::Error(QObject::tr("Could not allocate temporary picture"));
            }
            fill_image(ost.tmp_frame, image);
            sws_scale(ost.sws_context, (const uint8_t * const *) ost.tmp_frame->data,
                    ost.tmp_frame->linesize, 0, ost.codec_context->height, ost.frame->data,
                    ost.frame->linesize);
        }
        else
        {
            fill_image(ost.frame, image);
        }

        ost.frame->pts = ost.next_pts++;

        return ost.frame;
    }

    void write_video_frame(const QImage& image)
    {
        ost.write_frame(get_video_frame(image));
    }

    void flush()
    {
        ost.flush_frames();
    }

private:
    OutputStream ost;
};


class Logger
{
private:
    struct LogData
    {
        std::mutex mutex;
        io::video::VideoFormat* format = nullptr;
        int log_level;

        static LogData& instance()
        {
            static LogData instance;
            return instance;
        }

        static void static_callback(void *, int level, const char *fmt, va_list vl)
        {
            instance().callback(level, fmt, vl);
        }

        void setup(io::video::VideoFormat* format, int log_level)
        {
            auto guard = std::lock_guard(mutex);
            this->format = format;
            this->log_level = log_level;
            av_log_set_callback(&LogData::static_callback);
        }

        void teardown()
        {
            auto guard = std::lock_guard(mutex);
            av_log_set_callback(&av_log_default_callback);
            format = nullptr;
        }

        void callback(int level, const char *fmt, va_list vl)
        {
            auto guard = std::lock_guard(mutex);
            if ( level > log_level )
                return;
            char buffer[1024];
            std::vsprintf(buffer, fmt, vl);
            QString msg(buffer);
            if ( msg.endsWith('\n') )
                msg.remove(msg.size()-1, 1);

            if ( level > AV_LOG_WARNING )
            {
                format->information(msg);
            }
            else if ( level == AV_LOG_WARNING )
            {
                app::log::Log("libav").log(msg);
                format->warning(msg);
            }
            else
            {
                app::log::Log("libav").log(msg, app::log::Error);
                format->error(msg);
            }
        }
    };

public:
    Logger(io::video::VideoFormat* format)
        : Logger(format, av_log_get_level())
    {}

    Logger(io::video::VideoFormat* format, int log_level)
    {
        level = av_log_get_level();
        av_log_set_level(log_level);
        LogData::instance().setup(format, log_level);
    }

    ~Logger()
    {
        LogData::instance().teardown();
        av_log_set_level(level);
    }

private:
    int level;
};

class DeviceIo
{
public:
    DeviceIo(QIODevice* device, int block_size = 4*1024)
    {
        buffer = (unsigned char*)av_malloc(block_size);
        context_ = avio_alloc_context(
            buffer,
            block_size,
            1,
            device,
            &DeviceIo::read_packet,
            &DeviceIo::write_packet,
            &DeviceIo::seek
        );
    }

    ~DeviceIo()
    {
        avio_context_free(&context_);
        av_free(buffer);
    }

    AVIOContext* context() const noexcept
    {
        return context_;
    }

private:
    static int read_packet(void *opaque, uint8_t *buf, int buf_size)
    {
        QIODevice* device = (QIODevice*)opaque;
        return device->read((char*)buf, buf_size);
    }

    static int write_packet(void *opaque, uint8_t *buf, int buf_size)
    {
        QIODevice* device = (QIODevice*)opaque;
        return device->write((char*)buf, buf_size);
    }

    static int64_t seek(void *opaque, int64_t offset, int whence)
    {
        QIODevice* device = (QIODevice*)opaque;

        switch ( whence )
        {
            case SEEK_SET:
                return device->seek(offset);
            case SEEK_CUR:
                return device->seek(offset + device->pos());
            case SEEK_END:
                device->readAll();
                return device->seek(offset + device->pos());
        }
        return 0;
    }

    AVIOContext* context_;
    unsigned char * buffer;
};

} // namespace glaxnimate::av

glaxnimate::io::Autoreg<glaxnimate::io::video::VideoFormat> glaxnimate::io::video::VideoFormat::autoreg;


static QStringList out_ext;

static void get_formats()
{
    std::set<std::string> blacklisted = {
        "webp", "gif", "ico"
    };
    out_ext.push_back("mp4");

    void* opaque = nullptr;
    while ( auto format = av_muxer_iterate(&opaque) )
    {
        if (
            blacklisted.count(format->name) ||
            format->video_codec == AV_CODEC_ID_NONE ||
            format->flags & (AVFMT_NOFILE|AVFMT_NEEDNUMBER)
        )
            continue;

        out_ext += QString(format->extensions).split(',',
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            Qt::SkipEmptyParts
#else
            QString::SkipEmptyParts
#endif
        );
    }
}

QStringList glaxnimate::io::video::VideoFormat::extensions() const
{
    if ( out_ext.empty() )
        get_formats();
    return out_ext;
}

bool glaxnimate::io::video::VideoFormat::on_save(QIODevice& dev, const QString& name, model::Document* document, const QVariantMap& settings)
{
    try
    {
        av::Logger logger(this, settings["verbose"].toBool() ? AV_LOG_INFO : AV_LOG_WARNING);

        auto filename = name.toUtf8();


        av::Dict opt;
        /*
        for (int i = 2; i+1 < argc; i+=2) {
            if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
                av_dict_set(&opt, argv[i]+1, argv[i+1], 0);
        }
        */

        // allocate the output media context
        AVFormatContext *oc;

        QString format_hint = settings["format"].toString();
        if ( !format_hint.isEmpty() )
        {
            avformat_alloc_output_context2(&oc, nullptr, format_hint.toUtf8().data(), filename.data());
            if ( !oc )
            {
                error(tr("Format not supported: %1").arg(format_hint));
                return false;
            }
        }
        else
        {
            avformat_alloc_output_context2(&oc, nullptr, nullptr, filename.data());

            if ( !oc )
            {
                warning(tr("Could not deduce output format from file extension: using MPEG."));
                avformat_alloc_output_context2(&oc, nullptr, "mpeg", filename.data());
                if ( !oc )
                {
                    error(tr("Could not find output format"));
                    return false;
                }
            }
        }

        // see https://libav.org/documentation/doxygen/master/group__metadata__api.html
        av::DictWrapper metadata(&oc->metadata);
        metadata["title"] = document->main()->name.get();
        for ( auto it = document->metadata().begin(); it != document->metadata().end(); ++it )
            metadata[it.key()] = it->toString();

        av::CGuard guard(&avformat_free_context, oc);

        // Add the audio and video streams using the default format codecs
        // and initialize the codecs.
        if ( oc->oformat->video_codec == AV_CODEC_ID_NONE )
        {
            error(tr("No video codec"));
            return false;
        }

        // Now that all the parameters are set, we can open the audio and
        // video codecs and allocate the necessary encode buffers.
        int width = settings["width"].toInt();
        if ( width == 0 )
            width = document->main()->width.get();
        int height = settings["height"].toInt();
        if ( height == 0 )
            height = document->main()->height.get();
        int64_t bit_rate = settings["bit_rate"].toInt();
        int fps = qRound(document->main()->fps.get());
        av::Video video(oc, opt, bit_rate*100, width, height, fps);

        // log format info
        av_dump_format(oc, 0, filename, 1);

        // open the output file, if needed
        av::DeviceIo io(&dev);
        oc->pb = io.context();

        // Write the stream header, if any
        int ret = avformat_write_header(oc, opt.dict());
        if ( ret < 0 )
        {
            error(tr("Error occurred when opening output file: %1").arg(av::err2str(ret)));
            return false;
        }

        auto first_frame = document->main()->animation->first_frame.get();
        auto last_frame = document->main()->animation->last_frame.get();
        QColor background = settings["background"].value<QColor>();
        emit progress_max_changed(last_frame - first_frame);
        for ( auto i = first_frame; i < last_frame; i++ )
        {
            video.write_video_frame(document->render_image(i, {width, height}, background));
            emit progress(i - first_frame);
        }

        video.flush();

        // Write the trailer, if any. The trailer must be written before you
        // close the CodecContexts open when you wrote the header; otherwise
        // av_write_trailer() may try to use memory that was freed on
        // av_codec_close().
        av_write_trailer(oc);

        return true;
    }
    catch ( const av::Error& e )
    {
        error(e.message());
        return false;
    }
}

std::unique_ptr<app::settings::SettingsGroup> glaxnimate::io::video::VideoFormat::save_settings(model::Document*) const
{
    return std::make_unique<app::settings::SettingsGroup>(app::settings::SettingList{
        app::settings::Setting{"bit_rate",     "Bitrate",      "Video bit rate",                               5000,   0, 10000},
        app::settings::Setting{"background",   "Background",   "Background color",                             QColor{}},
        app::settings::Setting{"width",        "Width",        "If not 0, it will overwrite the size",         0,      0, 10000},
        app::settings::Setting{"height",       "Height",       "If not 0, it will overwrite the size",         0,      0, 10000},
        app::settings::Setting{"verbose",      "Verbose",      "Show verbose information on the conversion",   false},
    });
}

QString glaxnimate::io::video::VideoFormat::library_version()
{
    return QStringList{
        LIBAVUTIL_IDENT,
        LIBAVFORMAT_IDENT,
        LIBAVCODEC_IDENT,
        LIBSWSCALE_IDENT
    }.join(", ");
}
