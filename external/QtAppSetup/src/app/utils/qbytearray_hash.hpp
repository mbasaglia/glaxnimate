#pragma once

#include <QByteArray>
#include <QHash>

#include <functional>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace std {
  template<> struct hash<QByteArray> {
    std::size_t operator()(const QByteArray& s) const noexcept {
      return (size_t) qHash(s);
    }
  };
}
#endif

