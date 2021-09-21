#pragma once

#include <memory>

#include <QAbstractTableModel>
#include <QUrl>
#include <QRawFont>


namespace glaxnimate::gui::font {

class GoogleFontsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
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

        struct Variant
        {
            QUrl url;
            QRawFont font;
        };

        QString family;
        std::unordered_map<QString, Variant> variants;
        Category category;
        int popularity_index = 0;
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

    void refresh();
    bool has_token() const;

    int columnCount(const QModelIndex & parent = {}) const override;
    int rowCount(const QModelIndex & parent = {}) const override;
    QVariant data(const QModelIndex & index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;

signals:
    void max_progress_changed(int progess);
    void progress_changed(int progess);
    void error(const QString& message);

private:
    void response_progress(qint64 received, qint64 total);

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::font
