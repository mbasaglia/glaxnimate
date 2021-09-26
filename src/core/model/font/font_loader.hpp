#pragma once

#include <QByteArray>
#include "custom_font.hpp"

namespace glaxnimate::model {

enum class FontFileFormat
{
    Unknown,
    TrueType,
    OpenType,
    Woff2,
    Woff
};

FontFileFormat font_data_format(const QByteArray& data);

/// \todo move to gui and remove Qt::Networking from core cmake
class FontLoader : public QObject
{
    Q_OBJECT

public:
    FontLoader();
    ~FontLoader();

    /**
     * \brief Vector of loaded fonts
     */
    const std::vector<CustomFont>& fonts() const;

    /**
     * \brief Queue \p url for download
     * \param url Should point to a css, ttf, or otf file
     */
    void queue(const QUrl& url);

    /**
     * \brief Load fonts from the queue
     */
    void load_queue();

    /**
     * \brief Total number of queued fonts
     */
    int queued_total() const;

    /**
     * \brief Clears all data
     */
    void clear();

    /**
     * \brief Load from data
     * \param data Should be css, ttf, or otf
     */
    void queue_data(const QByteArray& data);

    /**
     * \brief Cancels all loads and clears all data
     */
    void cancel();

signals:
    /**
     * \brief All queued loads have been completed
     */
    void finished();

    /**
     * \brief Emitted when a font has been loaded
     * \param count number of URLs loaded so far
     */
    void fonts_loaded(int count);

    /**
     * \brief Emitted when something is queued
     */
    void fonts_queued(int total);

    /**
     * \brief Error message
     */
    void error(const QString& message);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::model
