#include <QtTest/QtTest>
#include <QPoint>

#include "model/property.hpp"

using namespace model;

struct PropertyChangedInspector
{
    PropertyChangedInspector() = default;
    PropertyChangedInspector(const PropertyChangedInspector&) = delete;
    PropertyChangedInspector& operator=(const PropertyChangedInspector&) = delete;

    void connect(Object& obj)
    {
        QObject::connect(&obj, &Object::property_changed, [this](const QString& name, const QVariant& value){(*this)(name, value);});
    }

    void connect_added(Object& obj)
    {
        QObject::connect(&obj, &Object::property_added, [this](const QString& name, const QVariant& value){(*this)(name, value);});
    }

    void operator()(const QString& name, const QVariant& value)
    {
        this->name = name;
        this->value = value;
        called = true;
    }

    bool not_called()
    {
        return !called;
    }

    bool called_with(const QString& name, const QVariant& value)
    {
        return called && this->name == name && this->value == value;
    }

    void reset()
    {
        name = QString{};
        value = {};
        called = false;
    }


    QString name;
    QVariant value;
    bool called = false;
};

class TestProperty: public QObject
{
    Q_OBJECT

private slots:

    void test_property_default()
    {
        PropertyChangedInspector pci;
        Object obj;
        pci.connect(obj);

        Property<int> prop(&obj, "foo", 456);
        QVERIFY(pci.not_called());
        QCOMPARE(prop.get(), 456);
    }

    void test_property_get_set()
    {
        PropertyChangedInspector pci;
        Object obj;
        pci.connect(obj);

        Property<int> prop(&obj, "foo");
        prop.set(123);
        QVERIFY(pci.called_with("foo", 123));
        QCOMPARE(prop.get(), 123);
    }

    void test_property_variant()
    {
        PropertyChangedInspector pci;
        Object obj;
        pci.connect(obj);

        Property<int> prop(&obj, "foo", 123);
        QCOMPARE(prop.value(), QVariant(123));
        QVERIFY(prop.set_value(QVariant(456)));
        QVERIFY(pci.called_with("foo", 456));
        QCOMPARE(prop.value(), QVariant(456));
        pci.reset();
        QVERIFY(!prop.set_value(QVariant(QPoint(1, 2))));
        QCOMPARE(prop.get(), 456);
        QVERIFY(pci.not_called());
    }

    void test_unknown_property_variant()
    {
        PropertyChangedInspector pci;
        Object obj;
        pci.connect(obj);

        UnknownProperty prop(&obj, "foo", 123);
        QCOMPARE(prop.value(), QVariant(123));
        QVERIFY(prop.set_value(QVariant(456)));
        QVERIFY(pci.called_with("foo", 456));
        QCOMPARE(prop.value(), QVariant(456));
    }

    void test_traits_get_type()
    {
        QCOMPARE(model::PropertyTraits::get_type<int>(), model::PropertyTraits::Int);
        QCOMPARE(model::PropertyTraits::get_type<float>(), model::PropertyTraits::Float);
        QCOMPARE(model::PropertyTraits::get_type<bool>(), model::PropertyTraits::Bool);
        QCOMPARE(model::PropertyTraits::get_type<QString>(), model::PropertyTraits::String);
        QCOMPARE(model::PropertyTraits::get_type<model::PropertyTraits::Type>(), model::PropertyTraits::Enum);
    }
};

QTEST_GUILESS_MAIN(TestProperty)
#include "test_property.moc"
