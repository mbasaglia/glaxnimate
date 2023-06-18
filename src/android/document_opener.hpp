/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DOCUMENT_OPENER_HPP
#define DOCUMENT_OPENER_HPP


#include <QWidget>

#include "model/document.hpp"

namespace glaxnimate::android {


class DocumentOpener
{
public:
    DocumentOpener(QWidget* widget_parent);
    ~DocumentOpener();

    bool save(const QUrl& url, model::Composition* composition, io::Options& options) const;

    std::unique_ptr<model::Document> open(const QUrl& url) const;

    std::unique_ptr<model::Document> from_raster(const QByteArray& data);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::android

#endif // DOCUMENT_OPENER_HPP
