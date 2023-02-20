/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QCborMap>
#include <QCborArray>

namespace glaxnimate::io::lottie {


QByteArray cbor_write_json(const QCborMap& obj, bool compact);

} // namespace glaxnimate::io::lottie
