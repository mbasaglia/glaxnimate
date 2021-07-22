#include "help_dialog.hpp"

#include <QScrollArea>
#include <QLabel>
#include <QGridLayout>
#include <QFrame>

#include "io/io_registry.hpp"
#include "scroll_area_event_filter.hpp"
#include "glaxnimate_app.hpp"

glaxnimate::android::HelpDialog::HelpDialog(QWidget *parent)
    : BaseDialog(parent)
{
    QGridLayout* ml = new QGridLayout(this);
    ml->setMargin(0);
    setLayout(ml);

    QScrollArea* area = new QScrollArea(this);
    ml->addWidget(area, 0, 0);
    (new ScrollAreaEventFilter(area))->setParent(area);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setWidgetResizable(true);

    QWidget* wid = new QWidget(area);
    area->setWidget(wid);

    QGridLayout* lay = new QGridLayout(wid);
    lay->setMargin(0);
    wid->setLayout(lay);

    QString supported_formats_import;
    for ( const auto& fmt : io::IoRegistry::instance().importers() )
    {
        if ( fmt->slug() == "raster" )
            continue;
        if ( !supported_formats_import.isEmpty() )
            supported_formats_import += "\n";
        supported_formats_import += " - " + fmt->name();
    }
    QString supported_formats_export = supported_formats_import;
    supported_formats_import += "\n - PNG (Needs to be traced into vector)";

    const std::vector<std::pair<QString, QString>> buttons = {
        {
            "document-new",
            tr("Clears the current document and creates a new empty one")
        },
        {
            "document-open",
            tr("Prompts the user to select a document to open.\nCurrently the following formats are supported:\n%1").arg(supported_formats_import)
        },
        {
            "document-import",
            tr("Prompts the user to select a document to append as an object to the current one.")
        },
        {
            "document-save",
            tr("Save the current document, prompting to select a file for new documents")
        },
        {
            "document-save-as",
            tr("Save the current document, always prompting to select a file.")
        },
        {
            "document-export",
            tr("Save a copy of current document, prompting to select a format and a file.\nCurrently the following formats are supported:\n%1").arg(supported_formats_export)
        },
        {
            "view-preview",
            tr("Saves the current frame as a still image")
        },
        {
            "document-send",
            tr("Creates a sticker pack to export to Telegram.\nNote that only recent version of Telegram support this.\nIf your Telegram version is too old, you can Export the file to TGS and upload that on Telegram.")
        },
        {
            "edit-cut",
            tr("Cuts the selection into the clipboard.")
        },
        {
            "edit-copy",
            tr("Copies the selection into the clipboard.")
        },
        {
            "edit-paste",
            tr("Pastes from the clipboard into the current document.")
        },
        {
            "edit-delete",
            tr("Removes the selected item.")
        },
        {
            "edit-undo",
            tr("Undoes the last action.")
        },
        {
            "edit-redo",
            tr("Redoes the last undone action.")
        },
        {
            "document-properties",
            tr("Opens the side pane used to change the advanced properties for the selected object.")
        },
        {
            "player-time",
            tr("Shows the timeline and playback controls.")
        },
        {
            "fill-color",
            tr("Opens the side pane used to select the fill color.")
        },
        {
            "object-stroke-style",
            tr("Opens the side pane used to select the stroke color and style.")
        },
        {
            "question",
            tr("Shows this help.")
        },
        {
            "edit-select",
            tr("Select tool, you can use it to select objects and change their transform.")
        },
        {
            "edit-node",
            tr("Edit tool, used to edit existing items (eg: moving, bezier nodes, setting rounded corners on a rectangle, etc.)")
        },
        {
            "draw-brush",
            tr("Shows a tray with the curve drawing tools.")
        },
        {
            "draw-bezier-curves",
            tr("Create Bezier curves using nodes and handles.")
        },
        {
            "draw-freehand",
            tr("Draw curves freehand.")
        },
        {
            "shapes",
            tr("Shows a tray with the shape drawing tools.")
        },
        {
            "draw-rectangle",
            tr("Draws rectangles.")
        },
        {
            "draw-ellipse",
            tr("Draws ellipses.")
        },
        {
            "draw-polygon-star",
            tr("Draws a star, after it's been created you can change it into a regular polygon and change the number of sides from the properties pane.")
        },
        {
            "draw-text",
            tr("Create and edit text shapes.")
        },
        {
            "overflow-menu",
            tr("Toggles the menu showing non-drawing buttons.")
        },
        {
            "media-playback-start",
            tr("Starts playback.")
        },
        {
            "media-playlist-loop",
            tr("Toggles looping for the playback (on by default).")
        },
        {
            "go-first",
            tr("Jumps to the first frame.")
        },
        {
            "go-previous",
            tr("Goes to the previous frame.")
        },
        {
            "go-next",
            tr("Goes to the next frame.")
        },
        {
            "go-last",
            tr("Jumps to the last frame.")
        },
        {
            "database-change-key",
            tr("When enabled (which is the default) whenever you change an object property, a new keyframe is added for that property.")
        },
        {
            "layer-lower",
            tr("Push the selection further back.")
        },
        {
            "layer-raise",
            tr("Brings the selection further to the front.")
        },
    };

    QSize pix_size(128, 128);

    int row = 0;

    QLabel* logo = new QLabel(wid);
    logo->setPixmap(QIcon(gui::GlaxnimateApp::instance()->data_file("images/splash.svg")).pixmap(pix_size * 2));
    logo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    lay->addWidget(logo, row++, 0, 1, 2, Qt::AlignCenter);

    QLabel* global_desc = new QLabel(wid);
    global_desc->setText(tr(R"(
Glaxnimate is a vector animation program.
You can use it to create and modify Animated SVG, Telegram Animated Stickers, and Lottie animations.
Follows a guide of the main icons and what they do.
)"));
    global_desc->setWordWrap(true);
    lay->addWidget(global_desc, row++, 0, 1, 2);

    for ( const auto& p : buttons )
    {
        QFrame *line = new QFrame(wid);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        lay->addWidget(line, row++, 0, 1, 2);

        QLabel* preview = new QLabel(wid);
        preview->setPixmap(QIcon::fromTheme(p.first).pixmap(pix_size));
        preview->setMinimumSize(pix_size);
        preview->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        lay->addWidget(preview, row, 0, Qt::AlignTop|Qt::AlignHCenter);

        QLabel* desc = new QLabel(wid);
        desc->setText(p.second);
        desc->setMinimumSize(pix_size);
        desc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        desc->setWordWrap(true);
        lay->addWidget(desc, row, 1);

        row++;
    }
}
