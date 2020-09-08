#pragma once

#include <QCborMap>
#include <QCborArray>

namespace io::lottie {


QByteArray cbor_write_json(const QCborMap& obj, bool compact);

} // namespace io::lottie
