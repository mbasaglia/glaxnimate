#pragma once

#include <QCborMap>
#include <QCborArray>

namespace glaxnimate::io::lottie {


QByteArray cbor_write_json(const QCborMap& obj, bool compact);

} // namespace glaxnimate::io::lottie
