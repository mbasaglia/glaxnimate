/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>

#include <QIODevice>
#include <QByteArray>

namespace glaxnimate::utils::gzip {

using ErrorFunc = std::function<void (const QString&)>;

bool decompress(QIODevice& input, QByteArray& output, const ErrorFunc& on_error);
bool decompress(const QByteArray& input, QByteArray& output, const ErrorFunc& on_error);
bool is_compressed(QIODevice& input);
bool is_compressed(const QByteArray& input);


} // namespace glaxnimate::utils::gzip
