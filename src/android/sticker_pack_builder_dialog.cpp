/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sticker_pack_builder_dialog.hpp"
#include "ui_sticker_pack_builder_dialog.h"

#include <QUuid>
#include <QMessageBox>
#include <QPainter>
#include <QLabel>
#include <QToolButton>
#include <QStandardPaths>
#include <QPointer>

#include "io/lottie/tgs_format.hpp"
#include "io/glaxnimate/glaxnimate_format.hpp"

#include "emoji/emoji_set.hpp"
#include "emoji/emoji_dialog.hpp"
#include "android_file_picker.hpp"
#include "document_opener.hpp"
#include "glaxnimate_app.hpp"
#include "telegram_intent.hpp"
#include "style/scroll_area_event_filter.hpp"


class glaxnimate::android::StickerPackBuilderDialog::Private
{
public:
    struct Item
    {
        Item(std::unique_ptr<model::Document> doc, Private* d)
            : document(std::move(doc)),
              emoji(emoji::EmojiGroup::table[0].first().unicode),
              d(d)
        {
            int row = d->ui.layout_items->rowCount();

            d->ui.layout_items->addWidget(&preview, row, 0);

            button_emoji.setText(emoji);
            connect(&button_emoji, &QAbstractButton::clicked, d->parent, [d, this]{
                d->change_emoji(this);
            });
            d->ui.layout_items->addWidget(&button_emoji, row, 1);

            d->ui.layout_items->addItem(&spacer, row, 2);

            button_delete.setIcon(QIcon::fromTheme("edit-delete"));
            connect(&button_delete, &QAbstractButton::clicked, d->parent, [d, this]{
                d->remove(this);
            });
            d->ui.layout_items->addWidget(&button_delete, row, 3);

            adjust_size();
        }

        ~Item()
        {
            d->ui.layout_items->removeWidget(&preview);
            d->ui.layout_items->removeWidget(&button_emoji);
            d->ui.layout_items->removeWidget(&button_delete);
            d->ui.layout_items->removeItem(&spacer);
        }

        void set_emoji(const QString& e)
        {
            emoji = e;
            button_emoji.setText(emoji);
        }

        void rebuild_preview()
        {
            rebuild_preview((
                document->main()->animation->first_frame.get() +
                document->main()->animation->last_frame.get()
            ) / 2);
        }

        void rebuild_preview(model::FrameTime t)
        {
            pixmap.fill(Qt::white);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            auto scale = qreal(pixmap.width()) / document->main()->width.get();
            painter.scale(scale, scale);
            document->main()->paint(&painter, t, model::VisualNode::Render);
            painter.end();
            preview.setPixmap(pixmap);
        }

        void adjust_size()
        {
            int width = d->extent;

            button_emoji.setFont(d->font);
            QSize size(width, width);
            preview.setFixedSize(size);
            button_emoji.setFixedSize(size);
            button_delete.setFixedSize(size);
            button_delete.setIconSize(QSize(width*0.8, width*0.8));
            pixmap = QPixmap(size);
            rebuild_preview();
        }

        std::unique_ptr<model::Document> document;
        QPixmap pixmap;
        QString emoji;

        QLabel preview;
        QPushButton button_emoji;
        QPushButton button_delete;
        QSpacerItem spacer{0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum};
        Private* d;
    };

    Private(StickerPackBuilderDialog* parent)
        : parent(parent), opener(parent)
    {
        ui.setupUi(parent);
        ui.button_add->setIcon(QIcon::fromTheme("document-open"));
        ui.button_export->setIcon(QIcon::fromTheme("document-send"));
        ui.button_clear->setIcon(QIcon::fromTheme("edit-clear-all"));
        ui.button_add_current->setIcon(QIcon::fromTheme("list-add"));
        ui.button_export->setEnabled(false);

        connect(&file_picker, &AndroidFilePicker::open_selected, parent, [this](const QUrl& url){
            add_document(url);
        });
        connect(ui.button_add, &QAbstractButton::clicked, parent, [this]{ request_add(); });
        connect(ui.button_export, &QAbstractButton::clicked, parent, [this]{ export_pack(); });
        connect(ui.button_add_current, &QAbstractButton::clicked, parent, [this]{ add_current(); });
        connect(ui.button_clear, &QAbstractButton::clicked, parent, [this]{ clear(); });

        update_extent();

        scroller.set_target(ui.scroll_area);
    }

    void update_extent()
    {
        extent = qMin(parent->height(), parent->width()) / 3.;
        font.setPixelSize(extent * 0.8);
    }

    void request_add()
    {
        if ( items.size() == 50 )
        {
            QMessageBox::warning(parent, parent->windowTitle(), tr("Too many stickers"));
            return;
        }

        if ( !file_picker.select_open(false) )
        {
            QMessageBox::warning(parent, parent->windowTitle(), tr("Could not open file"));
        }
    }

    void add_document(const QUrl& url)
    {
        auto doc = opener.open(url);
        if ( !doc )
        {
            QMessageBox::warning(parent, parent->windowTitle(), tr("Could not open file"));
            return;
        }

        ui.button_export->setEnabled(true);
        items.emplace_back(std::make_unique<Item>(std::move(doc), this));
    }

    void remove(Item* item)
    {
        for ( auto it = items.begin(); it != items.end(); ++it )
        {
            if ( it->get() == item )
            {
                items.erase(it);
                break;
            }
        }
        ui.button_export->setEnabled(!items.empty());
    }

    void change_emoji(Item* item)
    {
        for ( auto it = items.begin(); it != items.end(); ++it )
        {
            if ( it->get() == item )
            {
                if ( emoji_selector.exec() )
                    item->set_emoji(emoji_selector.current_unicode());
                return;
            }
        }
    }

    void export_pack()
    {
        QStringList filenames;
        QStringList emoji;

        QDir path_parent = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        QString subdir = QUuid::createUuid().toString(QUuid::WithoutBraces);
        path_parent.mkpath(subdir);
        QDir path = path_parent.absoluteFilePath(subdir);

        int index = 0;
        for ( const auto& item : items )
        {
            QString filename = path.absoluteFilePath(QString::number(index) + ".tgs");
            QFile file(filename);
            if ( !exporter.save(file, filename, item->document.get(), {}) )
            {
                QMessageBox::warning(parent, parent->windowTitle(), tr("Cannot save as TGS"));
                continue;
            }

            filenames << filename;
            emoji << item->emoji;

            index++;
        }

        auto result = TelegramIntent().send_stickers(filenames, emoji);
        if ( !result )
        {
            QMessageBox::warning(parent, parent->windowTitle(), result.message());
        }
        else
        {
            parent->accept();
        }
    }


    void add_current()
    {
        if ( !current_file )
            return;

        QByteArray data;
        QBuffer buf(&data);
        io::glaxnimate::GlaxnimateFormat format;
        format.save(buf, "", current_file, {});
        buf.seek(0);
        buf.close();
        std::unique_ptr<model::Document> new_file = std::make_unique<model::Document>("");
        format.open(buf, "", new_file.get(), {});
        items.emplace_back(std::make_unique<Item>(std::move(new_file), this));
        ui.button_export->setEnabled(true);
    }

    void clear()
    {
        if ( QMessageBox::question(parent, parent->windowTitle(), tr("Are you sure you want to remove all stickers from the pack?")) == QMessageBox::Yes )
        {
            items.clear();
            ui.button_export->setEnabled(false);
        }
    }

    qreal extent;
    Ui::StickerPackBuilderDialog ui;
    StickerPackBuilderDialog* parent;
    AndroidFilePicker file_picker;
    DocumentOpener opener;
    io::lottie::TgsFormat exporter;
    std::vector<std::unique_ptr<Item>> items;
    QFont font;
    emoji::EmojiDialog emoji_selector;
    QPointer<model::Document> current_file;
    gui::ScrollAreaEventFilter scroller;
    DialogFixerFilter fixer{&emoji_selector};
};

glaxnimate::android::StickerPackBuilderDialog::StickerPackBuilderDialog(QWidget *parent) :
    BaseDialog(parent),
    d(std::make_unique<Private>(this))
{
    d->emoji_selector.set_image_path(QDir("assets:/share/glaxnimate/glaxnimate/emoji/png"));
    emoji::EmojiSetSlugFormat slug;
    slug.prefix = "emoji_u";
    d->emoji_selector.set_image_suffix(".png");
    d->emoji_selector.set_image_slug_format(slug);
    d->emoji_selector.load_emoji(emoji::EmojiDialog::Image);
}

glaxnimate::android::StickerPackBuilderDialog::~StickerPackBuilderDialog()
{
}

void glaxnimate::android::StickerPackBuilderDialog::set_current_file(model::Document *current)
{
    d->current_file = current;
}

void glaxnimate::android::StickerPackBuilderDialog::changeEvent(QEvent *e)
{
    BaseDialog::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            d->ui.retranslateUi(this);
            break;
        default:
            break;
    }
}

void glaxnimate::android::StickerPackBuilderDialog::resizeEvent(QResizeEvent *e)
{
    BaseDialog::resizeEvent(e);
    d->update_extent();
    for ( const auto& item : d->items )
        item->adjust_size();
}

glaxnimate::emoji::EmojiDialog & glaxnimate::android::StickerPackBuilderDialog::emoji_dialog()
{
    return d->emoji_selector;
}
