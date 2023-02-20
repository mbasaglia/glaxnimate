/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>

#include <QDialog>
#include <QDir>


namespace glaxnimate::emoji {

struct EmojiSetSlugFormat;
struct EmojiSet;

class EmojiDialog : public QDialog
{
    Q_OBJECT

public:
    enum DisplayMode
    {
        Text,
        Image
    };

    EmojiDialog(QWidget* parent = nullptr);
    ~EmojiDialog();

    void set_emoji_font(const QFont& font);
    const QFont& emoji_font() const;

    void set_image_path(const QDir& path);
    const QDir& image_path() const;

    void set_image_suffix(const QString& suffix);
    const QString& image_suffix() const;

    void set_image_slug_format(const EmojiSetSlugFormat& slug);
    const EmojiSetSlugFormat& image_slug_format() const;

    void from_emoji_set(const EmojiSet& set, int size);

    void load_emoji(DisplayMode mode);

    QString current_unicode() const;
    QString current_slug() const;

signals:
    void selected_unicode(const QString& emoji);
    void selected_slug(const QString& emoji);

protected:
    void timerEvent(QTimerEvent *event) override;
    void showEvent(QShowEvent* e) override;
    void resizeEvent(QResizeEvent * event) override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::emoji
