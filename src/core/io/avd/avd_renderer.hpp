/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <functional>

#include <QDomDocument>
#include <QDomElement>


namespace glaxnimate::model {
    class Document;
} // namespace glaxnimate::model


namespace glaxnimate::io::avd {


class AvdRenderer
{
public:
    AvdRenderer(const std::function<void(const QString&)>& on_warning);
    ~AvdRenderer();

    void render(model::Document* document);

    QDomElement graphics();
    QDomDocument single_file();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::io::avd
