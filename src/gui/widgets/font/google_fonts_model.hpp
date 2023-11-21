/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <set>
#include <memory>
#include <unordered_map>

#include <QAbstractTableModel>
#include <QUrl>
#include <QRawFont>

#include "app/utils/qstring_hash.hpp"
#include "model/assets/embedded_font.hpp"


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
            Any,
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
            model::CustomFont font;

            int score(int weight, bool italic) const
            {
                return 1000 - std::abs(weight - this->weight) + (italic == this->italic ? 100 : 0);
            }

            bool operator< (const Style& other) const
            {
                return weight < other.weight || (weight == other.weight && italic < other.italic);
            }
        };

        QString css_url(const Style& style) const;

        using StyleList = std::vector<Style>;

        QString family;
        StyleList styles;
        Category category;
        int popularity_index = 0;
        DownloadStatus status = Available;
        std::set<QString> subsets;
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

    static QString category_name(GoogleFontsModel::GoogleFont::Category category);

    const GoogleFont* font(int row) const;
    void download_font(int row);

    const std::set<QString>& subsets() const;
    bool has_subset(const QModelIndex& index, const QString& subset) const;
    bool has_category(const QModelIndex& index, GoogleFont::Category cat) const;

Q_SIGNALS:
    void max_progress_changed(int progress);
    void progress_changed(int progress);
    void error(const QString& message);
    void download_finished(int row);
    void refresh_finished();

private:
    void response_progress(qint64 received, qint64 total);

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui::font
