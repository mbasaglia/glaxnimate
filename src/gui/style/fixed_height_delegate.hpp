#include <QStyledItemDelegate>

namespace style {

class FixedHeightDelegate : public QStyledItemDelegate
{
public:
    explicit FixedHeightDelegate(qreal height = -1) : height (height) {}

    void set_height(qreal height)
    {
        this->height = height;
    }

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override
    {
        auto size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(height);
        return size;
    }

private:
    qreal height;
};

} // namespace style
