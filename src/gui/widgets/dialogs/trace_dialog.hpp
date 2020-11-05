#ifndef TRACEDIALOG_H
#define TRACEDIALOG_H

#include <memory>
#include <QDialog>

namespace model {
class Image;
class DocumentNode;
} // namespace model

class TraceDialog : public QDialog
{
    Q_OBJECT

public:
    TraceDialog(model::Image* image, QWidget* parent = nullptr);
    ~TraceDialog();

    model::DocumentNode* created() const;

protected:
    void changeEvent ( QEvent* e ) override;
    void resizeEvent(QResizeEvent * event) override;

private slots:
    void update_preview();
    void apply();
    void change_mode(int mode);
    void add_color();
    void remove_color();
    void auto_colors();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // TRACEDIALOG_H
