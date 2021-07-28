#include "emoji_set_dialog.hpp"
#include "ui_emoji_set_dialog.h"

#include <QEvent>
#include <QUrl>
#include <QJsonDocument>
#include <QPainter>
#include <QSvgRenderer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDesktopServices>

#include "emoji/emoji_set.hpp"
#include "glaxnimate_app.hpp"

class glaxnimate::emoji::EmojiSetDialog::Private
{
public:
    Ui::EmojiSetDialog ui;
    std::vector<EmojiSet> sets;
    QNetworkAccessManager downloader;

    static const int preview_count = 6;
    static const int icon_size = 72;

    enum Colums
    {
        Name,
        License,
        Preview0,
        Downloaded = Preview0 + preview_count,
    };

    void set_download_status(int row, const char* theme_icon, const QString& msg)
    {
        auto item_loaded = new QTableWidgetItem();
        item_loaded->setIcon(QIcon::fromTheme(theme_icon));
        item_loaded->setToolTip(msg);
        ui.emoji_set_view->setItem(row, Private::Downloaded, item_loaded);
    }
};

glaxnimate::emoji::EmojiSetDialog::EmojiSetDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.emoji_set_view->verticalHeader()->setVisible(false);
    d->ui.emoji_set_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    d->ui.emoji_set_view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    d->ui.emoji_set_view->verticalHeader()->setMaximumSectionSize(Private::icon_size);
    d->ui.emoji_set_view->verticalHeader()->setMinimumSectionSize(Private::icon_size);
    d->ui.emoji_set_view->verticalHeader()->setDefaultSectionSize(Private::icon_size);
    d->ui.emoji_set_view->setIconSize(QSize(Private::icon_size, Private::icon_size));


    d->ui.progress_bar->setVisible(false);

    reload_sets();
}

glaxnimate::emoji::EmojiSetDialog::~EmojiSetDialog() = default;

void glaxnimate::emoji::EmojiSetDialog::changeEvent ( QEvent* e )
{
    QWidget::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void glaxnimate::emoji::EmojiSetDialog::reload_sets()
{
    d->ui.emoji_set_view->setRowCount(0);

    auto sets_path = gui::GlaxnimateApp::instance()->data_file("emoji/sets.json");
    if ( sets_path.isEmpty() )
        return;

    QFile sets_file(sets_path);
    if ( !sets_file.open(QFile::ReadOnly) )
        return;

    QJsonDocument doc = QJsonDocument::fromJson(sets_file.readAll());
    sets_file.close();

    d->sets.clear();

    for ( const auto& val : doc.array() )
        d->sets.push_back(EmojiSet::load(val.toObject()));


    static const std::array<QString, Private::preview_count> preview_emoji = {
        "1f61c",        // ;P
        "1f336",        // hot pepper
        "1f432",        // dragon face
        "1f1ea-1f1fa",  // EU flag
        "1faa2",        // knot
        "2764",         // red heart
    };

    int row = 0;
    for ( auto& p : d->sets )
    {
        p.path = QDir(gui::GlaxnimateApp::instance()->writable_data_path("emoji/" + p.name));
        d->ui.emoji_set_view->setRowCount(row+1);
        d->ui.emoji_set_view->setItem(row, Private::Name, new QTableWidgetItem(p.name));
        d->ui.emoji_set_view->setItem(row, Private::License, new QTableWidgetItem(p.license));

        for ( int i = 0; i < Private::preview_count; i++ )
        {
            auto reply = d->downloader.get(QNetworkRequest(p.preview_url(preview_emoji[i])));
            connect(reply, &QNetworkReply::finished, this, [this, row, i, reply]{
                if ( reply->error() )
                    return;
                QPixmap pix(Private::icon_size, Private::icon_size);
                pix.fill(Qt::transparent);
                QPainter painter(&pix);
                QSvgRenderer renderer(reply->readAll());
                renderer.render(&painter);
                if ( !pix.isNull() )
                {
                    auto item = new QTableWidgetItem;
                    item->setData(Qt::DecorationRole, pix);
                    item->setSizeHint(pix.size() + QSize(5, 5));
                    d->ui.emoji_set_view->setItem(row, Private::Preview0 + i, item);
                }
            });
        }

        if ( p.path.exists() )
            d->set_download_status(row, "package-installed-updated", tr("Installed"));
        else
            d->set_download_status(row, "package-available", tr("Not Installed"));

        row++;
    }
}

void glaxnimate::emoji::EmojiSetDialog::download_selected()
{
    int row = d->ui.emoji_set_view->currentRow();
    if ( row < 0 || row >= int(d->sets.size()) )
        return;

    QNetworkRequest request(d->sets[row].download.url);
    request.setMaximumRedirectsAllowed(3);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto reply = d->downloader.get(request);
    d->ui.progress_bar->setVisible(true);
    d->ui.progress_bar->setValue(0);
    d->ui.progress_bar->setMaximum(100);
    d->set_download_status(row, "package-installed-outdated", tr("Downloading"));
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total){
        d->ui.progress_bar->setValue(received);
        d->ui.progress_bar->setMaximum(total);
    });
    connect(reply, &QNetworkReply::finished, this, [this, row, reply]{
        d->ui.progress_bar->setVisible(false);
        if ( reply->error() || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 )
        {
            auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
            d->set_download_status(row, "package-broken", tr("Could not download: %1").arg(reason));
            return;
        }

        QByteArray data = reply->readAll();
        reply->close();

        QFile debug("/tmp/" + d->sets[row].name + ".tar.gz");
        debug.open(QFile::WriteOnly);
        debug.write(data);
        d->set_download_status(row, "package-installed-updated", tr("Installed"));
    });
}


void glaxnimate::emoji::EmojiSetDialog::set_selected(int row)
{
    bool enabled = row >= 0 && row < int(d->sets.size());
    d->ui.button_add_emoji->setEnabled(enabled);
    d->ui.button_download->setEnabled(enabled);
    d->ui.button_view_website->setEnabled(enabled);
}

void glaxnimate::emoji::EmojiSetDialog::view_website()
{
    int row = d->ui.emoji_set_view->currentRow();
    if ( row < 0 || row >= int(d->sets.size()) )
        return;
    QDesktopServices::openUrl(d->sets[row].url);
}
