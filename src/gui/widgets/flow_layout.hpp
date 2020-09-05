#pragma once

#include <vector>

#include <QLayout>
#include <QStyle>


class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(int items_per_row = 3, int min_w = 32, int max_w = 80, QWidget *parent = nullptr);
    ~FlowLayout();

    void addItem(QLayoutItem *item) override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

private:
    int do_layout(const QRect &rect, bool test_only) const;
    bool valid_index(int index) const;

    std::vector<QLayoutItem *> items;
    int min_w;
    int max_w;
    int items_per_row;
};