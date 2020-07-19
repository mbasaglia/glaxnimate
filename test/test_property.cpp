#define BOOST_TEST_MODULE Test
#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_CASE( test_property_default )
{
    PropertyChangedInspector pci;
    Object obj;
    pci.connect(obj);

    Property<int> prop(&obj, "foo", "bar", 456);
    BOOST_TEST(pci.not_called());
    BOOST_TEST(prop.get() == 456);
}

BOOST_AUTO_TEST_CASE( test_property_get_set )
{
    PropertyChangedInspector pci;
    Object obj;
    pci.connect(obj);

    Property<int> prop(&obj, "foo", "bar");
    prop.set(123);
    BOOST_TEST(pci.called_with("foo", 123));
    BOOST_TEST(prop.get() == 123);
}

BOOST_AUTO_TEST_CASE( test_property_variant )
{
    PropertyChangedInspector pci;
    Object obj;
    pci.connect(obj);

    Property<int> prop(&obj, "foo", "bar", 123);
    BOOST_CHECK(prop.value() == QVariant(123));
    BOOST_CHECK(prop.set_value(QVariant(456)));
    BOOST_TEST(pci.called_with("foo", 456));
    BOOST_CHECK(prop.value() == QVariant(456));
    pci.reset();
    BOOST_CHECK(!prop.set_value(QVariant(QPoint(1, 2))));
    BOOST_TEST(prop.get() == 456);
    BOOST_CHECK(pci.not_called());
}

BOOST_AUTO_TEST_CASE( test_unknown_property_variant )
{
    PropertyChangedInspector pci;
    Object obj;
    pci.connect(obj);

    UnknownProperty prop(&obj, "foo", 123);
    BOOST_CHECK(prop.value() == QVariant(123));
    BOOST_CHECK(prop.set_value(QVariant(456)));
    BOOST_TEST(pci.called_with("foo", 456));
    BOOST_CHECK(prop.value() == QVariant(456));
}

BOOST_AUTO_TEST_CASE( test_traits_get_type )
{
    BOOST_CHECK(model::PropertyTraits::get_type<int>() == model::PropertyTraits::Int);
    BOOST_CHECK(model::PropertyTraits::get_type<float>() == model::PropertyTraits::Float);
    BOOST_CHECK(model::PropertyTraits::get_type<bool>() == model::PropertyTraits::Bool);
    BOOST_CHECK(model::PropertyTraits::get_type<QString>() == model::PropertyTraits::String);
    BOOST_CHECK(model::PropertyTraits::get_type<model::PropertyTraits::Type>() == model::PropertyTraits::Enum);
}
