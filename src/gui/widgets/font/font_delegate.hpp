#pragma once

#include <QAbstractItemDelegate>
#include <QFontDatabase>

namespace font {


class FontDelegate : public QAbstractItemDelegate
{
public:
    explicit FontDelegate(QObject *parent = nullptr);

    // painting
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

//     const QIcon truetype;
//     const QIcon bitmap;
    QFontDatabase::WritingSystem writingSystem;
};

} // namespace font
