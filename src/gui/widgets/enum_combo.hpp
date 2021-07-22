#pragma once

#include <QComboBox>
#include <QMetaEnum>

namespace glaxnimate::gui {


class EnumCombo : public QComboBox
{
    Q_OBJECT

public:
    EnumCombo(QWidget* parent = nullptr);
    EnumCombo(const QMetaEnum& meta_enum, int current_value, QWidget* parent = nullptr);

    template<class T>
    EnumCombo(T current_value, QWidget* parent = nullptr)
    : EnumCombo(QMetaEnum::fromType<T>(), int(current_value), parent)
    {}

    int current_value() const;
    void set_current_value(int i);

    void set_data(const QMetaEnum& meta_enum, int current_value);
    bool set_data_from_qvariant(const QVariant& var);


    static std::pair<QString, const char*> data_for(const QVariant& data);
    static std::pair<QString, const char*> data_for(const QMetaEnum& meta_enum, int value);

private:
    static bool inspect_qvariant(const QVariant& data, QMetaEnum& meta_enum, int& value);

    void populate(int current_value);
    void retranslate();

    QMetaEnum meta_enum;
};

} // namespace glaxnimate::gui
