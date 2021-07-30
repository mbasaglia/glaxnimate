#pragma once

#include <memory>

#include <QDialog>

#include "io/base.hpp"

namespace glaxnimate::gui {

class IoStatusDialog : public QDialog
{
    Q_OBJECT

public:
    IoStatusDialog(const QIcon& icon, const QString& title, bool delete_on_close, QWidget* parent = nullptr);
    ~IoStatusDialog();

    void reset(io::ImportExport* ie, const QString& label);
    void disconnect_import_export();

    bool has_errors() const;
    void show_errors(const QString& success, const QString& failure);

protected:
    void closeEvent(QCloseEvent* ev) override;
    void changeEvent(QEvent *e) override;

private:
    void _on_error(const QString& message, app::log::Severity severity);
    void _on_progress_max_changed(int max);
    void _on_progress(int value);
    void _on_completed(bool success);

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
