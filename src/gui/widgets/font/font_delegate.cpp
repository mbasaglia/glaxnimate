#include "font_delegate.hpp"

#include <QPainter>

using namespace glaxnimate::gui;
using namespace glaxnimate;

static QFontDatabase::WritingSystem writingSystemFromScript(QLocale::Script script)
{
    switch (script) {
    case QLocale::ArabicScript:
        return QFontDatabase::Arabic;
    case QLocale::CyrillicScript:
        return QFontDatabase::Cyrillic;
    case QLocale::GurmukhiScript:
        return QFontDatabase::Gurmukhi;
    case QLocale::SimplifiedHanScript:
        return QFontDatabase::SimplifiedChinese;
    case QLocale::TraditionalHanScript:
        return QFontDatabase::TraditionalChinese;
    case QLocale::LatinScript:
        return QFontDatabase::Latin;
    case QLocale::ArmenianScript:
        return QFontDatabase::Armenian;
    case QLocale::BengaliScript:
        return QFontDatabase::Bengali;
    case QLocale::DevanagariScript:
        return QFontDatabase::Devanagari;
    case QLocale::GeorgianScript:
        return QFontDatabase::Georgian;
    case QLocale::GreekScript:
        return QFontDatabase::Greek;
    case QLocale::GujaratiScript:
        return QFontDatabase::Gujarati;
    case QLocale::HebrewScript:
        return QFontDatabase::Hebrew;
    case QLocale::JapaneseScript:
        return QFontDatabase::Japanese;
    case QLocale::KhmerScript:
        return QFontDatabase::Khmer;
    case QLocale::KannadaScript:
        return QFontDatabase::Kannada;
    case QLocale::KoreanScript:
        return QFontDatabase::Korean;
    case QLocale::LaoScript:
        return QFontDatabase::Lao;
    case QLocale::MalayalamScript:
        return QFontDatabase::Malayalam;
    case QLocale::MyanmarScript:
        return QFontDatabase::Myanmar;
    case QLocale::TamilScript:
        return QFontDatabase::Tamil;
    case QLocale::TeluguScript:
        return QFontDatabase::Telugu;
    case QLocale::ThaanaScript:
        return QFontDatabase::Thaana;
    case QLocale::ThaiScript:
        return QFontDatabase::Thai;
    case QLocale::TibetanScript:
        return QFontDatabase::Tibetan;
    case QLocale::SinhalaScript:
        return QFontDatabase::Sinhala;
    case QLocale::SyriacScript:
        return QFontDatabase::Syriac;
    case QLocale::OriyaScript:
        return QFontDatabase::Oriya;
    case QLocale::OghamScript:
        return QFontDatabase::Ogham;
    case QLocale::RunicScript:
        return QFontDatabase::Runic;
    case QLocale::NkoScript:
        return QFontDatabase::Nko;
    default:
        return QFontDatabase::Any;
    }
}
static QFontDatabase::WritingSystem writingSystemFromLocale()
{
    QStringList uiLanguages = QLocale::system().uiLanguages();
    QLocale::Script script;
    if (!uiLanguages.isEmpty())
        script = QLocale(uiLanguages.at(0)).script();
    else
        script = QLocale::system().script();

    return writingSystemFromScript(script);
}

static QFontDatabase::WritingSystem writingSystemForFont(const QFont &font, bool *hasLatin)
{
    QList<QFontDatabase::WritingSystem> writingSystems = QFontDatabase().writingSystems(font.family());

    // this just confuses the algorithm below. Vietnamese is Latin with lots of special chars
    writingSystems.removeOne(QFontDatabase::Vietnamese);
    *hasLatin = writingSystems.removeOne(QFontDatabase::Latin);

    if (writingSystems.isEmpty())
        return QFontDatabase::Any;

    QFontDatabase::WritingSystem system = writingSystemFromLocale();

    if (writingSystems.contains(system))
        return system;

    if (system == QFontDatabase::TraditionalChinese
            && writingSystems.contains(QFontDatabase::SimplifiedChinese)) {
        return QFontDatabase::SimplifiedChinese;
    }

    if (system == QFontDatabase::SimplifiedChinese
            && writingSystems.contains(QFontDatabase::TraditionalChinese)) {
        return QFontDatabase::TraditionalChinese;
    }

    system = writingSystems.constLast();

    if (!*hasLatin) {
        // we need to show something
        return system;
    }

    if (writingSystems.count() == 1 && system > QFontDatabase::Cyrillic)
        return system;

    if (writingSystems.count() <= 2 && system > QFontDatabase::Armenian && system < QFontDatabase::Vietnamese)
        return system;

    if (writingSystems.count() <= 5 && system >= QFontDatabase::SimplifiedChinese && system <= QFontDatabase::Korean)
        return system;

    return QFontDatabase::Any;
}



font::FontDelegate::FontDelegate(QObject *parent)
    : QAbstractItemDelegate(parent),
//       truetype(QStringLiteral(":/qt-project.org/styles/commonstyle/images/fonttruetype-16.png")),
//       bitmap(QStringLiteral(":/qt-project.org/styles/commonstyle/images/fontbitmap-16.png")),
      writingSystem(QFontDatabase::Any)
{
}


void font::FontDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    QFont font(option.font);
    font.setPointSize(QFontInfo(font).pointSize() * 3 / 2);
    QFont font2 = font;
    font2.setFamily(text);

    bool hasLatin;
    QFontDatabase::WritingSystem system = writingSystemForFont(font2, &hasLatin);
    if (hasLatin)
        font = font2;

    QRect r = option.rect;

    if (option.state & QStyle::State_Selected) {
        painter->save();
        painter->setBrush(option.palette.highlight());
        painter->setPen(Qt::NoPen);
        painter->drawRect(option.rect);
        painter->setPen(QPen(option.palette.highlightedText(), 0));
    }

    /*
    const QIcon *icon = &bitmap;
    if (QFontDatabase().isSmoothlyScalable(text)) {
        icon = &truetype;
    }
    const QSize actualSize = icon->actualSize(r.size());
    const QRect iconRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                               actualSize, r);
    icon->paint(painter, iconRect, Qt::AlignLeft|Qt::AlignVCenter);
    if (option.direction == Qt::RightToLeft)
        r.setRight(r.right() - actualSize.width() - 4);
    else
        r.setLeft(r.left() + actualSize.width() + 4);
*/
    QFont old = painter->font();
    painter->setFont(font);

    const Qt::Alignment textAlign = QStyle::visualAlignment(option.direction, option.displayAlignment);
    // If the ascent of the font is larger than the height of the rect,
    // we will clip the text, so it's better to align the tight bounding rect in this case
    // This is specifically for fonts where the ascent is very large compared to
    // the descent, like certain of the Stix family.
    QFontMetricsF fontMetrics(font);
    if (fontMetrics.ascent() > r.height()) {
        QRectF tbr = fontMetrics.tightBoundingRect(text);
        QRect textRect(r);
        textRect.setHeight(textRect.height() + (r.height() - tbr.height()));
        painter->drawText(textRect, Qt::AlignBottom|Qt::TextSingleLine|textAlign, text);
    } else {
        painter->drawText(r, Qt::AlignVCenter|Qt::TextSingleLine|textAlign, text);
    }

    if (writingSystem != QFontDatabase::Any)
        system = writingSystem;

    if (system != QFontDatabase::Any) {
        int w = painter->fontMetrics().horizontalAdvance(text + QLatin1String("  "));
        painter->setFont(font2);
        QString sample = QFontDatabase().writingSystemSample(system);
        if (option.direction == Qt::RightToLeft)
            r.setRight(r.right() - w);
        else
            r.setLeft(r.left() + w);
        painter->drawText(r, Qt::AlignVCenter|Qt::TextSingleLine|textAlign, sample);
    }
    painter->setFont(old);

    if (option.state & QStyle::State_Selected)
        painter->restore();

}

QSize font::FontDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    QFont font(option.font);
//     font.setFamily(text);
    font.setPointSize(QFontInfo(font).pointSize() * 3/2);
    QFontMetrics fontMetrics(font);
    return QSize(fontMetrics.horizontalAdvance(text), fontMetrics.height());
}
