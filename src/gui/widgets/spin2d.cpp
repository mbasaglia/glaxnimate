#include "spin2d.hpp"

#include <QHBoxLayout>
#include <QStyleOptionSpinBox>
#include "glaxnimate_app.hpp"

using namespace glaxnimate::gui;


Spin2D::Spin2D(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* lay = new QHBoxLayout(this);
    setLayout(lay);
    spin_x = new SmallerSpinBox(true, this);
    void (QDoubleSpinBox::*sig)(qreal) = &QDoubleSpinBox::valueChanged;
    connect(spin_x, sig, this, &Spin2D::x_changed);
    lay->addWidget(spin_x);
    spin_y = new SmallerSpinBox(true, this);
    connect(spin_y, sig, this, &Spin2D::y_changed);
    lay->addWidget(spin_y);
    lay->setContentsMargins(0, 0, 0, 0);

}

Spin2D::Spin2D(bool ratio_lock, QWidget* parent)
    : Spin2D(parent)
{
    if ( ratio_lock )
        enable_ratio_lock();
}

void Spin2D::enable_ratio_lock()
{
    if ( lock )
        return;

    lock = new QToolButton(this);
    lock->setCheckable(true);
#ifdef Q_OS_ANDROID
    lock->setIconSize(QSize(50, 50));
#endif
    lock_toggled(false);
    connect(lock, &QToolButton::clicked, this, &Spin2D::lock_toggled);
    static_cast<QHBoxLayout*>(layout())->addWidget(lock);
    retranslate();
}

int SmallerSpinBox::get_spin_size(const QAbstractSpinBox* box)
{
    static int spin_size;
    if ( spin_size == 0 )
    {
        const QFontMetrics fm(box->fontMetrics());
        QString s = "999.99";
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        int w = qMax(0, fm.boundingRect(s).width());
#else
        int w = qMax(0, fm.horizontalAdvance(s));
#endif
        w += 2; // cursor blinking space

        QStyleOptionSpinBox option;
        option.initFrom(box);
        QSize hint(w, box->height());
        option.subControls = QStyle::SC_SpinBoxEditField | QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
        option.frame = box->hasFrame();
        spin_size = box->style()->sizeFromContents(QStyle::CT_SpinBox, &option, hint, box).expandedTo(QApplication::globalStrut()).width();
    }

    return spin_size;
}

void Spin2D::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if ( e->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
}

void Spin2D::retranslate()
{
    if ( lock )
        lock->setToolTip(tr("Lock Ratio"));
}

void Spin2D::lock_toggled(bool on)
{
    if ( on )
    {
        if ( y() != 0 )
            ratio = x() / y();
        lock->setIcon(GlaxnimateApp::theme_icon("object-locked"));
    }
    else
    {
        lock->setIcon(GlaxnimateApp::theme_icon("object-unlocked"));
    }
}

bool Spin2D::ratio_lock() const
{
    return lock && lock->isChecked();
}

void Spin2D::set_value(const QPointF& v)
{
    set_value(v.x(), v.y());
}

void Spin2D::set_value(const QSizeF& v)
{
    spin_x->setMinimum(0);
    spin_y->setMinimum(0);
    set_value(v.width(), v.height());
}

void Spin2D::set_value(const QVector2D& v)
{
    spin_x->setSuffix(tr("%"));
    spin_y->setSuffix(tr("%"));
    spin_x->setDecimals(0);
    spin_y->setDecimals(0);
    set_value(v.x() * 100, v.y() * 100);
}

void Spin2D::set_value(qreal x, qreal y)
{
    bool bx = spin_x->blockSignals(true);
    bool by = spin_y->blockSignals(true);
    spin_x->setValue(x);
    spin_y->setValue(y);
    if ( ratio_lock() && y != 0 )
        ratio = x / y;
    spin_x->blockSignals(bx);
    spin_y->blockSignals(by);
}

qreal Spin2D::x() const
{
    return spin_x->value();
}

qreal Spin2D::y() const
{
    return spin_y->value();
}

QPointF Spin2D::value_point() const
{
    return {x(), y()};
}

QSizeF Spin2D::value_size() const
{
    return {x(), y()};
}

QVector2D Spin2D::value_vector() const
{
    return QVector2D(x() / 100, y() / 100);
}

void Spin2D::x_changed(qreal x)
{
    if ( ratio_lock() && ratio != 0)
    {
        bool b = spin_y->blockSignals(true);
        spin_y->setValue(x / ratio);
        spin_y->blockSignals(b);
    }
    emit value_changed();
}

void Spin2D::y_changed(qreal y)
{
    if ( ratio_lock() && ratio != 0)
    {
        bool b = spin_x->blockSignals(true);
        spin_x->setValue(y * ratio);
        spin_x->blockSignals(b);
    }
    emit value_changed();
}

void Spin2D::set_decimals(int decimals)
{
    spin_x->setDecimals(decimals);
    spin_y->setDecimals(decimals);
}
