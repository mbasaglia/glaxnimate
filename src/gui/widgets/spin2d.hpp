#pragma once

#include <QToolButton>
#include <QVector2D>
#include "smaller_spinbox.hpp"

class Spin2D : public QWidget
{
    Q_OBJECT
    
public:
    Spin2D(bool ratio_lock, QWidget* parent = nullptr);
    Spin2D(QWidget* parent = nullptr);
    
    QVector2D value_vector() const;
    QPointF value_point() const;
    QSizeF value_size() const;

    void enable_ratio_lock();
    
    void set_value(const QVector2D& v);
    void set_value(const QPointF& v);
    void set_value(const QSizeF& v);
    void set_value(qreal x, qreal y);

    void set_decimals(int decimals);
    
    qreal x() const;
    qreal y() const;
    
protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void lock_toggled(bool on);
    void x_changed(qreal x);
    void y_changed(qreal y);

signals:
    void value_changed();
    
private:
    bool ratio_lock() const;
    void retranslate();
    
    qreal ratio = 1;
    SmallerSpinBox* spin_x;
    SmallerSpinBox* spin_y;
    QToolButton* lock = nullptr;
};
