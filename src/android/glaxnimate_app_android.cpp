#include "glaxnimate_app_android.hpp"

#include <QScreen>

static qreal get_mult()
{
#ifdef Q_OS_ANDROID_FAKE
    return 1;
#else
    auto sz = QApplication::primaryScreen()->size();
    return qMin(sz.width(), sz.height()) / 240.;
#endif
}

qreal GlaxnimateApp::handle_size_multiplier()
{
    static qreal mult = get_mult();
    return mult;
}

qreal GlaxnimateApp::handle_distance_multiplier()
{
    return handle_size_multiplier() / 2.;
}
