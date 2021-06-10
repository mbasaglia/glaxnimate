#ifndef SNIPPETLISTWIDGET_H
#define SNIPPETLISTWIDGET_H

#include <memory>
#include <QWidget>


class SnippetListWidget : public QWidget
{
    Q_OBJECT

public:
    SnippetListWidget(QWidget* parent = nullptr);
    ~SnippetListWidget();

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void snippet_new();
    void snippet_edit();
    void snippet_delete();
    void snippet_run();

signals:
    void run_snippet(const QString& source);

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // SNIPPETLISTWIDGET_H
