/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EMOJISETDIALOG_H
#define EMOJISETDIALOG_H

#include <memory>
#include <QDialog>

namespace glaxnimate::emoji {

class EmojiSetDialog : public QDialog
{
    Q_OBJECT

public:
    EmojiSetDialog(QWidget* parent = nullptr);
    ~EmojiSetDialog();

    const QString& selected_svg() const;

protected:
    void changeEvent ( QEvent* e ) override;

private slots:
    void reload_sets();
    void download_selected();
    void set_selected(int row);
    void view_website();
    void add_emoji();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::emoji

#endif // EMOJISETDIALOG_H
