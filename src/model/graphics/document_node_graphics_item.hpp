#pragma once

#include <QGraphicsObject>

namespace model::graphics {

class DocumentNodeGraphicsItem : public QGraphicsObject
{
    Q_OBJECT

public slots:
    void set_visible(bool v)
    {
        setVisible(v);
    }

signals:
    void focused(DocumentNodeGraphicsItem* item);

protected:
    void focusInEvent(QFocusEvent *) override
    {
        emit focused(this);
    }
};

} // namespace model::graphics
