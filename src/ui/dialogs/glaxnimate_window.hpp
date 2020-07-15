#ifndef GLAXNIMATEWINDOW_H
#define GLAXNIMATEWINDOW_H

#include <QMainWindow>

class GlaxnimateWindowPrivate;

class GlaxnimateWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * Default constructor
     */
    GlaxnimateWindow();

    /**
     * Destructor
     */
    ~GlaxnimateWindow();

private:
    GlaxnimateWindowPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(GlaxnimateWindow)
};

#endif // GLAXNIMATEWINDOW_H
