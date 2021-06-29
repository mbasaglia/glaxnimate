#include <QApplication>

#include <QMainWindow>
#include <QSplashScreen>
#include <QLabel>

// #include "math/bezier/bezier.hpp"
// #include "model/document.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSplashScreen sc;
    sc.setPixmap(QPixmap(":glaxnimate/splash.svg"));
    sc.show();
    app.processEvents();

//    app.initialize();

//    std::vector<int> v;
//    v.push_back(1);
    QMainWindow window;
    sc.finish(&window);
    window.show();

    QLabel l(&window);
    l.setText("Foo Bar");
    l.show();

//     math::bezier::Bezier bez;
//     bez.split_segment(0, 0);

//     model::Document doc("");
//     l.setText(doc.main()->objectName() + "vasfsdhkj");

    int ret = app.exec();
    return ret;
}
