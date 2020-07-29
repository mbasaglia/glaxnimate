#include "translation_service.hpp"

#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>

#include "app/application.hpp"

void app::TranslationService::initialize ( QString default_lang_code )
{
    if ( !default_lang_code.isEmpty() )
    {
        QString name = language_name(default_lang_code);
        if ( !name.isEmpty() )
            register_translation(name,default_lang_code, QString());
    }

    QDir translations = Application::instance()->data_file("translations");
    QStringList translation_files = translations.entryList({"*.qm"});

    QRegExp re("[^_]+_([^.]+)\\.qm");
    foreach ( QString file, translation_files )
    {
        if ( re.exactMatch(file) )
        {
            QString code = re.cap(1);
            QString name = language_name(code);
            if ( !name.isEmpty() )
                register_translation(name,code,translations.absoluteFilePath(file));
        }
//         else
//             qWarning() << tr("Warning:")
//                        << tr("Unrecognised translation file name pattern: %1")
//                           .arg(file);
    }

    change_lang_code(QLocale::system().name());
}

QString app::TranslationService::language_name(QString lang_code)
{
    QLocale lang_loc = QLocale(lang_code);
    QString name = QLocale::languageToString(lang_loc.language()); // English name

    if ( !name.isEmpty() )
        name[0] = name[0].toUpper();

    return name;
}


void app::TranslationService::register_translation(QString name, QString code, QString file)
{
    lang_names[name] = code;
    if ( !file.isEmpty() )
    {
        translators[code] = new QTranslator;
        if ( !translators[code]->load(file) )
        {
//             qWarning() << tr("Warning:") <<
//             /*: %1 is the file name,
//              *  %2 is the human-readable language code
//              *  %3 is the ISO language code
//              */
//             tr("Error on loading translation file %1 for language %2 (%3)")
//             .arg(file).arg(name).arg(code);
        }
    }
}



QString app::TranslationService::current_language_name()
{
    return lang_names.key(current_language);
}

QString app::TranslationService::current_language_code()
{
    return current_language;
}

QTranslator *app::TranslationService::translator()
{
    return translators[current_language];
}

const QMap<QString, QString>& app::TranslationService::available_languages()
{
    return lang_names;
}

void app::TranslationService::change_lang_code(QString code)
{

    if ( !translators.contains(code) )
    {
        QString base_code = code.left(code.lastIndexOf('_')); // en_US -> en
        bool found = false;
        foreach ( QString installed_code, translators.keys() )
        {
            if ( installed_code.left(installed_code.lastIndexOf('_')) == base_code )
            {
                code = installed_code;
                found = true;
                break;
            }
        }
        if ( not found )
        {
            /*qWarning() << tr("Warning:") <<
                          tr("There is no translation for language %1 (%2)")
                          .arg(language_name(code))
                          .arg(code);*/
            return;
        }
    }

    QCoreApplication* app = QCoreApplication::instance();
    app->removeTranslator(translator());
    current_language = code;
    app->installTranslator(translator());
}
