#pragma once

#include <memory>
#include <unordered_map>

#include <QAbstractTableModel>
#include <QUrl>
#include <QRawFont>

#include "app/utils/qstring_hash.hpp"


namespace glaxnimate::gui::font {

class GoogleFontsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    static constexpr int SortRole = Qt::UserRole;

    struct GoogleFont
    {
        enum Category
        {
            Display,
            Handwriting,
            Monospace,
            SansSerif,
            Serif,
        };

        enum DownloadStatus
        {
            InProgress,
            Broken,
            Downloaded,
            Available,
        };

        struct Style
        {
            QUrl url;
            int weight = 400;
            bool italic = false;
            int font_database_index = -1;
            QString font_database_family = {};

            int score(int weight, bool italic) const
            {
                return 1000 - std::abs(weight - this->weight) + (italic == this->italic ? 100 : 0);
            }

            bool operator< (const Style& other) const
            {
                return weight < other.weight || (weight == other.weight && italic < other.italic);
            }
        };

        using StyleList = std::vector<Style>;

        QString family;
        StyleList styles;
        Category category;
        int popularity_index = 0;
        DownloadStatus status = Available;
    };

    enum Column
    {
        Family,
        Category,
        Popularity,
        Status,

        Count
    };

    GoogleFontsModel();
    ~GoogleFontsModel();

    QString style_name(const GoogleFont::Style& slug) const;

    void refresh();
    bool has_token() const;

    int columnCount(const QModelIndex & parent = {}) const override;
    int rowCount(const QModelIndex & parent = {}) const override;
    QVariant data(const QModelIndex & index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;

//     void download_font(const QString& family);

//     const GoogleFont* font(const QString& family) const;
    const GoogleFont* font(int row) const;
    void download_font(int row);

signals:
    void max_progress_changed(int progess);
    void progress_changed(int progess);
    void error(const QString& message);
    void download_finished(int row);

private:
    void response_progress(qint64 received, qint64 total);

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::font
