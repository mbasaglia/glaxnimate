#pragma once

#include <QByteArray>
#include "model/custom_font.hpp"
#include "model/document.hpp"

namespace glaxnimate::gui::font {

class FontLoader : public QObject
{
    Q_OBJECT

public:
    FontLoader();
    ~FontLoader();

    /**
     * \brief Vector of loaded fonts
     */
    const std::vector<model::CustomFont>& fonts() const;

    /**
     * \brief Queue \p url for download
     * \param name_alias Name of the font family (if any)
     * \param url Should point to a css, ttf, or otf file
     * \param id Custom identifier to recognize when loading has finished
     */
    void queue(const QString& name_alias, const QUrl& url, int id = -1);

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
     * \param name_alias Name of the font family (if any)
     * \param data Should be css, ttf, or otf
     * \param id Custom identifier to recognize when loading has finished
     */
    void queue_data(const QString& name_alias, const QByteArray& data, int id = -1);

    /**
     * \brief Cancels all loads and clears all data
     */
    void cancel();

    /**
     * \brief Whether loading is in progress
     */
    bool loading() const;

    /**
     * \brief Queues all pending assets from the document
     */
    void queue_pending(model::Document* document, bool reload_loaded = false);

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
    void error(const QString& message, int id);

    /**
     * \brief Success for resource with the given ID
     */
    void success(int id);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::model
