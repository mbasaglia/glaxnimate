#pragma once

#include <memory>
#include <vector>

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

private:
    using container = std::vector<BaseProperty*>;

public:
    class iterator
    {
    public:
        const BaseProperty& operator*() const { return **iter; }
        const BaseProperty* operator->() const { return *iter; }
        bool operator==(const iterator& oth) const { return iter == oth.iter; }
        bool operator!=(const iterator& oth) const { return iter != oth.iter; }
        iterator& operator++()
        {
            ++iter;
            return *this;
        }
    private:
        iterator(const container::const_iterator& iter) : iter(iter) {}

        container::const_iterator iter;
        friend Object;
    };

    Object();
    ~Object();

    virtual std::unique_ptr<Object> clone() const;

    QVariant get(const QString& property) const;
    bool set(const QString& property, const QVariant& value, bool allow_unknown = false);

    iterator begin() const;
    iterator end() const;

signals:
    void property_added(const QString& name, const QVariant& value);
    void property_changed(const QString& name, const QVariant& value);

protected:
    void clone_into(Object* dest) const;

private:
    void add_property(BaseProperty* prop);
    void property_value_changed(const QString& name, const QVariant& value);

    friend BaseProperty;
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace model
