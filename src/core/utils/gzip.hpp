#pragma once

#include <functional>
#include <QIODevice>
#include <QByteArray>

namespace utils::gzip {

using ErrorFunc = std::function<void (const QString&)>;

bool compress(const QByteArray& input, QIODevice& output, const ErrorFunc& on_error, int level = 9);
bool decompress(QIODevice& input, QByteArray& output, const ErrorFunc& on_error);
bool decompress(const QByteArray& input, QByteArray& output, const ErrorFunc& on_error);


} // namespace utils::gzip
