#include "video_format.hpp"

#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "app/qstring_exception.hpp"
#include "app/log/log.hpp"

namespace av {

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
    AVStream *st = nullptr;
    AVCodecContext *enc = nullptr;

    // pts of the next frame that will be generated
    int64_t next_pts = 0;
    int samples_count = 0;

    AVFrame *frame = nullptr;
    AVFrame *tmp_frame = nullptr;

    struct SwsContext *sws_ctx = nullptr;
};

int flush_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st)
{
    int written = 0;
    int ret = 0;

    while ( ret >= 0 )
    {
        AVPacket pkt;
        memset(&pkt, 0, sizeof(AVPacket));

        ret = avcodec_receive_packet(c, &pkt);
        if ( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
            break;
        else if (ret < 0)
            throw av::Error(QObject::tr("Error encoding a frame: %1").arg(av::err2str(ret)));

        written++;
        // rescale output packet timestamp values from codec to stream timebase
        av_packet_rescale_ts(&pkt, c->time_base, st->time_base);
        pkt.stream_index = st->index;

        // Write the compressed frame to the media file.
        ret = av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0)
            throw av::Error(QObject::tr("Error while writing output packet: %1").arg(av::err2str(ret)));
    }

    return written;
}

static int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                       AVStream *st, AVFrame *frame)
{
    // send the frame to the encoder
    int ret = avcodec_send_frame(c, frame);
    if (ret < 0)
        throw av::Error(QObject::tr("Error sending a frame to the encoder: %1").arg(av::err2str(ret)));

    return flush_frame(fmt_ctx, c, st);
}

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


    Video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
        : oc(oc), ost(ost)
    {
        int ret;
        AVCodecContext *c = ost->enc;
        AVDictionary *opt = nullptr;

        av_dict_copy(&opt, opt_arg, 0);

        // open the codec
        ret = avcodec_open2(c, codec, &opt);
        av_dict_free(&opt);
        if (ret < 0)
            throw av::Error(QObject::tr("Could not open video codec: %1").arg(av::err2str(ret)));

        // allocate and init a re-usable frame
        ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
        if (!ost->frame)
            throw av::Error(QObject::tr("Could not allocate video frame"));

        ost->tmp_frame = nullptr;

        /* copy the stream parameters to the muxer */
        ret = avcodec_parameters_from_context(ost->st->codecpar, c);
        if (ret < 0)
            throw av::Error(QObject::tr("Could not copy the stream parameters"));
    }

    /* Prepare a dummy image. */
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
        AVCodecContext *c = ost->enc;

        // when we pass a frame to the encoder, it may keep a reference to it
        // internally; make sure we do not overwrite it here
        if ( av_frame_make_writable(ost->frame) < 0 )
            throw av::Error(QObject::tr("Error while creating video frame"));

        auto format = image_format(image.format());
        if ( format.first == AV_PIX_FMT_NONE )
        {
            image = QImage(c->width, c->height, QImage::Format_RGB888);
            format.first = AV_PIX_FMT_RGB24;
        }
        else if ( format.second != image.format() )
        {
            image = image.convertToFormat(format.second);
        }

        if ( c->pix_fmt != format.first || image.width() != c->width || image.height() != c->height )
        {
            if (!ost->sws_ctx)
            {
                ost->sws_ctx = sws_getContext(
                    image.width(), image.height(), format.first,
                    c->width, c->height, c->pix_fmt,
                    /// \todo parameter for scale
                    SWS_BICUBIC,
                    nullptr, nullptr, nullptr
                );
                if (!ost->sws_ctx)
                    throw av::Error(QObject::tr("Could not initialize the conversion context"));
            }
            if ( !ost->tmp_frame )
            {
                ost->tmp_frame = alloc_picture(format.first, image.width(), image.height());
                if (!ost->tmp_frame)
                    throw av::Error(QObject::tr("Could not allocate temporary picture"));
            }
            fill_image(ost->tmp_frame, image);
            sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,
                    ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                    ost->frame->linesize);
        }
        else
        {
            fill_image(ost->frame, image);
        }

        ost->frame->pts = ost->next_pts++;

        return ost->frame;
    }

    void write_video_frame(const QImage& image)
    {
        int written = write_frame(oc, ost->enc, ost->st, get_video_frame(image));
        skipped += 1 - written;
    }

    void flush()
    {
        for ( int i = 0; i < skipped; )
        {
            ost->frame->pts = ost->next_pts++;
            int c = write_frame(oc, ost->enc, ost->st, ost->frame);
            if ( c == 0 )
                break;
            i += c;
        }
    }

    ~Video()
    {
        avcodec_free_context(&ost->enc);
        av_frame_free(&ost->frame);
        av_frame_free(&ost->tmp_frame);
        sws_freeContext(ost->sws_ctx);
    }

private:
    AVFormatContext *oc;
    OutputStream *ost;
    int skipped = 0;
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

// Add an output stream.
void add_stream(
    av::OutputStream *ost,
    AVFormatContext *oc,
    AVCodec **codec,
    AVCodecID codec_id,
    model::Document* doc
)
{
    AVCodecContext *c;

    // find the encoder
    *codec = avcodec_find_encoder(codec_id);
    if ( !*codec )
        throw av::Error(QObject::tr("Could not find encoder for '%1'").arg(avcodec_get_name(codec_id)));

    ost->st = avformat_new_stream(oc, nullptr);
    if (!ost->st)
        throw av::Error(QObject::tr("Could not allocate stream"));

    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c)
        throw av::Error(QObject::tr("Could not alloc an encoding context"));
    ost->enc = c;

    if ( (*codec)->type == AVMEDIA_TYPE_VIDEO )
    {
        c->codec_id = codec_id;

        /// \todo setting
        c->bit_rate = 400000;
        /// \todo size settings
        // Resolution must be a multiple of two
        c->width = doc->main()->width.get();
        if ( c->width % 2 )
            c->width -= 1;
        c->height = doc->main()->height.get();
        if ( c->height % 2 )
            c->height -= 1;
        // timebase: This is the fundamental unit of time (in seconds) in terms
        // of which frame timestamps are represented. For fixed-fps content,
        // timebase should be 1/framerate and timestamp increments should be
        // identical to 1.
        ost->st->time_base = AVRational{ 1, qRound(doc->main()->fps.get()) };
        c->time_base = ost->st->time_base;

        // emit one intra frame every twelve frames at most
        c->gop_size = 12;
        if ( (*codec)->pix_fmts == nullptr )
            throw av::Error(QObject::tr("Could not determine pixel format"));
        // get_format() for some reason returns an invalid value
        c->pix_fmt = (*codec)->pix_fmts[0];

        // just for testing, we also add B-frames
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO)
            c->max_b_frames = 2;
    }

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

} // namespace av

io::Autoreg<io::video::VideoFormat> io::video::VideoFormat::autoreg;

QStringList io::video::VideoFormat::extensions() const
{
    return {"mp4"};
}

bool io::video::VideoFormat::on_save(QIODevice& dev, const QString& name, model::Document* document, const QVariantMap& settings)
{
    dev.close();

    try
    {
        av::Logger logger(this, settings["verbose"].toBool() ? AV_LOG_INFO : AV_LOG_WARNING);

        auto filename = name.toUtf8();


        AVDictionary *opt = nullptr;
        /*
        for (int i = 2; i+1 < argc; i+=2) {
            if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
                av_dict_set(&opt, argv[i]+1, argv[i+1], 0);
        }
        */

        // allocate the output media context
        AVFormatContext *oc;
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

        av::CGuard guard(&avformat_free_context, oc);

        // Add the audio and video streams using the default format codecs
        // and initialize the codecs.
        if ( oc->oformat->video_codec == AV_CODEC_ID_NONE )
        {
            error(tr("No video codec"));
            return false;
        }

        av::OutputStream video_st;
        AVCodec *video_codec;
        add_stream(&video_st, oc, &video_codec, oc->oformat->video_codec, document);

        // Now that all the parameters are set, we can open the audio and
        // video codecs and allocate the necessary encode buffers.
        av::Video video(oc, video_codec, &video_st, opt);

        av_dump_format(oc, 0, filename, 1);

        // open the output file, if needed
        if ( !(oc->oformat->flags & AVFMT_NOFILE) )
        {
            int ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
            if ( ret < 0 )
            {
                error(tr("Could not open '%1': %2").arg(name).arg(av::err2str(ret)));
                return false;
            }
        }

        // Write the stream header, if any
        int ret = avformat_write_header(oc, &opt);
        if ( ret < 0 )
        {
            error(tr("Error occurred when opening output file: %1").arg(av::err2str(ret)));
            return false;
        }

        auto first_frame = document->main()->animation->first_frame.get();
        auto last_frame = document->main()->animation->last_frame.get();
        emit progress_max_changed(last_frame - first_frame);
        for ( auto i = first_frame; i < last_frame; i++ )
        {
            video.write_video_frame(document->render_image(i));
            emit progress(i - first_frame);
        }

        video.flush();

        // Write the trailer, if any. The trailer must be written before you
        // close the CodecContexts open when you wrote the header; otherwise
        // av_write_trailer() may try to use memory that was freed on
        // av_codec_close().
        av_write_trailer(oc);

        // Close codec.
        if ( !(oc->oformat->flags & AVFMT_NOFILE) )
            avio_closep(&oc->pb);

        return true;
    }
    catch ( const av::Error& e )
    {
        error(e.message());
        return false;
    }
}

io::SettingList io::video::VideoFormat::save_settings() const
{
    return {
        io::Setting{"verbose", "Verbose", "Show verbose information on the conversion", false}
    };
}

