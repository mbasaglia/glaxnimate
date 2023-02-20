/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ANIMATIONPROPERTIESDIALOG_H
#define ANIMATIONPROPERTIESDIALOG_H

#include <memory>
#include <QDialog>
#include <QAbstractButton>

namespace glaxnimate::model { class Document; }

namespace glaxnimate::gui {

class TimingDialog : public QDialog
{
    Q_OBJECT

public:
    TimingDialog(model::Document* document, QWidget* parent = nullptr);
    ~TimingDialog();

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void btn_clicked(QAbstractButton* button);
    void changed_seconds(double s);
    void changed_frames(int f);
    void changed_fps(double fps);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // ANIMATIONPROPERTIESDIALOG_H
