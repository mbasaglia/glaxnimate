#pragma once

#include <QDoubleSpinBox>

class SmallerSpinBox : public QDoubleSpinBox
{
public:
    SmallerSpinBox(bool adaptive, QWidget* parent = nullptr)
        : QDoubleSpinBox(parent)
    {
        setMinimum(-999'999.99); // '); lupdate is sometimes weird
        setMaximum(+999'999.99); // '); lupdate is sometimes weird
        setValue(0);
        setDecimals(2);
        
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        resize(sizeHint());
        
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        if ( adaptive )
            setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
#endif
    }
    
    QSize sizeHint() const override
    {
        QSize sh = QDoubleSpinBox::sizeHint();
        sh.setWidth(get_spin_size(this));
        return sh;
    }
    
    static int get_spin_size(const QAbstractSpinBox* box);
};


class SmallerSpinBoxInt : public QSpinBox
{
public:
    SmallerSpinBoxInt(QWidget* parent = nullptr)
        : QSpinBox(parent)
    {
        setMinimum(-999'999); // '); lupdate is sometimes weird
        setMaximum(+999'999); // '); lupdate is sometimes weird
        setValue(0);
        
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        resize(sizeHint());
    }
    
    QSize sizeHint() const override
    {
        QSize sh = QSpinBox::sizeHint();
        sh.setWidth(SmallerSpinBox::get_spin_size(this));
        return sh;
    }
};
