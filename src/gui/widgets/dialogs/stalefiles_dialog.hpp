#ifndef GLAXNIMATE_GUI_STALEFILESDIALOG_H
#define GLAXNIMATE_GUI_STALEFILESDIALOG_H

#include <memory>
#include <QDialog>
#include <KAutoSaveFile>

namespace glaxnimate {
namespace gui {

class StalefilesDialog : public QDialog
{
    Q_OBJECT

public:
    StalefilesDialog(const QList<KAutoSaveFile*>& stale, QWidget* parent = nullptr);
    ~StalefilesDialog();

    KAutoSaveFile* selected() const;

    void cleanup(KAutoSaveFile* keep);

private slots:
    void delete_all();
    void delete_selected();
    void current_changed(int index);

protected:
    void changeEvent ( QEvent* e ) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

}}

#endif // GLAXNIMATE_GUI_STALEFILESDIALOG_H
