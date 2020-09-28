#include <QtTest/QtTest>
#include <QPoint>
#include <QMetaProperty>

#include "model/property/object_list_property.hpp"
#include "model/document.hpp"

using namespace model;

struct PropertyChangedInspector
{
    PropertyChangedInspector() = default;
    PropertyChangedInspector(const PropertyChangedInspector&) = delete;
    PropertyChangedInspector& operator=(const PropertyChangedInspector&) = delete;

    void connect(Object& obj)
    {
        QObject::connect(&obj, &Object::property_changed, [this](const BaseProperty* prop, const QVariant& value){(*this)(prop->name(), value);});
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

class MetaTestSubject : public DocumentNode
{
    Q_OBJECT

    GLAXNIMATE_PROPERTY(int, prop_scalar, 123)
    GLAXNIMATE_PROPERTY_REFERENCE(MetaTestSubject, prop_ref, &MetaTestSubject::valid_references, &MetaTestSubject::is_valid_reference)
    GLAXNIMATE_PROPERTY_LIST(MetaTestSubject, prop_list, nullptr, nullptr, nullptr, nullptr)

public:
    QIcon docnode_icon() const override { return {}; }
    DocumentNode* docnode_parent() const override { return {}; }
    int docnode_child_count() const override { return {}; }
    DocumentNode* docnode_child(int) const override { return {}; }
    int docnode_child_index(DocumentNode*) const override { return -1; }

    std::vector<ReferenceTarget*> valid_references() const
    {
        return {
            const_cast<MetaTestSubject*>(this)
        };
    }
    bool is_valid_reference(ReferenceTarget* p)
    {
        return p == this;
    }

    using DocumentNode::DocumentNode;

    double foo_val(int bar) { return bar / 2.0; }
    double foo_val_const(int bar) const { return bar / 2.0; }
    double foo_ref(const int& bar) { return bar / 2.0; }
    double foo_ref_const(const int& bar) const { return bar / 2.0; }

    QRectF local_bounding_rect(FrameTime) const override { return {}; }
};


class TestProperty: public QObject
{
    Q_OBJECT

private slots:

    void test_property_default()
    {
        PropertyChangedInspector pci;
        Object obj(nullptr);
        pci.connect(obj);

        Property<int> prop(&obj, "foo", 456);
        QVERIFY(pci.not_called());
        QCOMPARE(prop.get(), 456);
    }

    void test_property_get_set()
    {
        PropertyChangedInspector pci;
        Object obj(nullptr);
        pci.connect(obj);

        Property<int> prop(&obj, "foo");
        prop.set(123);
        QVERIFY(pci.called_with("foo", 123));
        QCOMPARE(prop.get(), 123);
    }

    void test_property_variant()
    {
        PropertyChangedInspector pci;
        Object obj(nullptr);
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

    void test_traits_get_type()
    {
        QCOMPARE(model::PropertyTraits::get_type<int>(), model::PropertyTraits::Int);
        QCOMPARE(model::PropertyTraits::get_type<float>(), model::PropertyTraits::Float);
        QCOMPARE(model::PropertyTraits::get_type<bool>(), model::PropertyTraits::Bool);
        QCOMPARE(model::PropertyTraits::get_type<QString>(), model::PropertyTraits::String);
        QCOMPARE(model::PropertyTraits::get_type<model::PropertyTraits::Type>(), model::PropertyTraits::Enum);
    }

    void test_metaobject()
    {
        Document doc("foo");
        MetaTestSubject test_subject(&doc);
        MetaTestSubject other(&doc);

        QMetaObject mop = DocumentNode::staticMetaObject;
        QMetaObject mo = MetaTestSubject::staticMetaObject;

        QCOMPARE(mo.propertyCount(), mop.propertyCount() + 3);

        QMetaProperty prop_0 = mo.property(mop.propertyCount() + 0);
        QCOMPARE(prop_0.name(), "prop_scalar");
        QVERIFY(prop_0.isWritable());
        QVERIFY(prop_0.isReadable());
        QVERIFY(prop_0.isScriptable());
        QCOMPARE(prop_0.read(&test_subject).toInt(), 123);
        QVERIFY(prop_0.write(&test_subject, QVariant(621)));
        QCOMPARE(prop_0.read(&test_subject).toInt(), 621);
        QCOMPARE(test_subject.prop_scalar.get(), 621);
        QVERIFY(!prop_0.write(&test_subject, QVariant("xyz")));
        QCOMPARE(test_subject.prop_scalar.get(), 621);

        QMetaProperty prop_1 = mo.property(mop.propertyCount() + 1);
        QCOMPARE(prop_1.name(), "prop_ref");
        QVERIFY(prop_1.isWritable());
        QVERIFY(prop_1.isReadable());
        QVERIFY(prop_1.isScriptable());
        QCOMPARE((quintptr)prop_1.read(&test_subject).value<MetaTestSubject*>(), 0);
        QVERIFY(prop_1.write(&test_subject, QVariant::fromValue(&test_subject)));
        QCOMPARE((quintptr)prop_1.read(&test_subject).value<MetaTestSubject*>(), (quintptr)&test_subject);
        QCOMPARE((quintptr)test_subject.prop_ref.get(), (quintptr)&test_subject);
        // invalid references shouldn't be set
        prop_1.write(&test_subject, QVariant::fromValue(&other));
        QCOMPARE((quintptr)test_subject.prop_ref.get(), (quintptr)&test_subject);

        QMetaProperty prop_2 = mo.property(mop.propertyCount() + 2);
        QCOMPARE(prop_2.name(), "prop_list");
        QVERIFY(!prop_2.isWritable());
        QVERIFY(prop_2.isReadable());
        QVERIFY(prop_2.isScriptable());
        QCOMPARE(prop_2.read(&test_subject).toList().size(), 0);
        test_subject.prop_list.insert(std::make_unique<MetaTestSubject>(&doc));
        QCOMPARE(test_subject.prop_list.size(), 1);
        QCOMPARE(prop_2.read(&test_subject).toList().size(), 1);
        QCOMPARE(prop_2.read(&test_subject).toList()[0].value<MetaTestSubject*>(), test_subject.prop_list[0]);
    }

    void test_callback_val()
    {
        Document doc("foo");
        MetaTestSubject test_subject(&doc);
        PropertyCallback<double, int> pc;
        QVERIFY(!pc);
        pc = &MetaTestSubject::foo_val;
        QVERIFY(pc);
        QCOMPARE(pc(&test_subject, 3), 1.5);
        pc = nullptr;
        QVERIFY(!pc);
    }

    void test_callback_val_const()
    {
        Document doc("foo");
        MetaTestSubject test_subject(&doc);
        PropertyCallback<double, int> pc;
        QVERIFY(!pc);
        pc = &MetaTestSubject::foo_val_const;
        QVERIFY(pc);
        QCOMPARE(pc(&test_subject, 3), 1.5);
        pc = nullptr;
        QVERIFY(!pc);
    }

    void test_callback_ref()
    {
        Document doc("foo");
        MetaTestSubject test_subject(&doc);
        PropertyCallback<double, int> pc;
        QVERIFY(!pc);
        pc = &MetaTestSubject::foo_ref;
        QVERIFY(pc);
        QCOMPARE(pc(&test_subject, 3), 1.5);
        pc = nullptr;
        QVERIFY(!pc);
    }

    void test_callback_ref_const()
    {
        Document doc("foo");
        MetaTestSubject test_subject(&doc);
        PropertyCallback<double, int> pc;
        QVERIFY(!pc);
        pc = &MetaTestSubject::foo_ref_const;
        QVERIFY(pc);
        QCOMPARE(pc(&test_subject, 3), 1.5);
        pc = nullptr;
        QVERIFY(!pc);
    }
};

QTEST_GUILESS_MAIN(TestProperty)
#include "test_property.moc"
