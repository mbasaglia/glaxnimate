#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <memory>
#include <QDialog>

#include "model/document.hpp"

class StartupDialog : public QDialog
{
    Q_OBJECT

public:
    StartupDialog(QWidget* parent = nullptr);
    ~StartupDialog();

    std::unique_ptr<model::Document> create() const;

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void reload_presets();
    void select_preset(const QModelIndex& index);
    void click_recent(const QModelIndex& index);
    void update_time_units();
    void update_startup_enabled(bool checked);

signals:
    void open_recent(const QString& path);
    void open_browse();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // STARTUPDIALOG_H
