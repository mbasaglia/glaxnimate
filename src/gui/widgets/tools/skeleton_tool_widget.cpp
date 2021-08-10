#include "skeleton_tool_widget.hpp"
#include "ui_skeleton_tool_widget.h"

#include <QEvent>

class glaxnimate::gui::SkeletonToolWidget::Private
{
public:
    Ui::SkeletonToolWidget ui;
};

glaxnimate::gui::SkeletonToolWidget::SkeletonToolWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

glaxnimate::gui::SkeletonToolWidget::~SkeletonToolWidget() = default;

void glaxnimate::gui::SkeletonToolWidget::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}
