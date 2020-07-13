#pragma once

#include <memory>

#include <QObject>
#include <QVariant>

namespace std {
  template<> struct hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
      return (size_t) qHash(s);
    }
  };
}



namespace model {

class BaseProperty;

class Object : public QObject
{
    Q_OBJECT

public:
    Object();
    ~Object();

    QVariant get(const QString& property) const;
    bool set(const QString& property, const QVariant& value, bool allow_unknown = false);

signals:
    void property_changed(const QString& name, const QVariant& value);

private:
    void add_property(BaseProperty* prop);
    void property_value_changed(const QString& name, const QVariant& value);

    friend BaseProperty;
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
