#include "glaxnimate_window.hpp"
#include "glaxnimate_window_p.hpp"


GlaxnimateWindow::GlaxnimateWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), d(std::make_unique<Private>())
{
    d->setupUi(this);
}

GlaxnimateWindow::~GlaxnimateWindow() = default;


void GlaxnimateWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            d->retranslateUi(this);
            break;
        default:
            break;
    }
}

bool GlaxnimateWindow::eventFilter(QObject* object, QEvent* event)
{
    QToolButton *btn = qobject_cast<QToolButton*>(object);
    if ( btn && event->type() == QEvent::Resize )
    {
        int target = btn->size().width() - 10;
        QSize best(0, 0);
        for ( const auto& sz : btn->icon().availableSizes() )
        {
            if ( sz.width() > best.width() && sz.width() <= target )
                best = sz;
        }
        if ( best.width() > 0 )
            btn->setIconSize(best);
    }

    return false;
}
