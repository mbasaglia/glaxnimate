/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "translation_service.hpp"

#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QRegularExpression>

#include "app/application.hpp"
#include "app/log/log.hpp"

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

    QRegularExpression re("[^_]+_([^.]+)\\.qm");
    for (auto &file: std::as_const(translation_files))
    {
        auto match = re.match(file);
        if ( match.hasMatch() )
        {
            QString code = match.captured(1);
            QString name = language_name(code);
            if ( !name.isEmpty() )
                register_translation(name,code,translations.absoluteFilePath(file));
        }
        else
        {
            log::LogStream("Translations") << "Unrecognised translation file name pattern:" << file;
        }
    }

    change_lang_code(QLocale::system().name());
}

QString app::TranslationService::language_name(QString lang_code)
{
    QLocale lang_loc = QLocale(lang_code);
    QString name = lang_loc.nativeLanguageName();
    QString specifier;

    if ( lang_code.contains("_") )
    {
        if ( lang_loc.script() != QLocale::AnyScript )
            specifier = QLocale::scriptToString(lang_loc.script());

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        if ( lang_loc.territory() != QLocale::AnyTerritory )
        {
            if ( !specifier.isEmpty() )
                specifier += ", ";
            specifier = lang_loc.nativeTerritoryName();
        }
#else
        if ( lang_loc.country() != QLocale::AnyCountry )
        {
            if ( !specifier.isEmpty() )
                specifier += ", ";
            specifier = lang_loc.nativeCountryName();
        }
#endif
    }

    if ( !name.isEmpty() )
    {
        name[0] = name[0].toUpper();
        if ( !specifier.isEmpty() )
            name += " (" + specifier + ")";
    }

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
            log::Log("Translations").log(
                QString("Error on loading translation file %1 for language %2 (%3)")
                .arg(file).arg(name).arg(code)
            );
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
        for (const auto &installed_code: translators.keys())
        {
            if ( installed_code.left(installed_code.lastIndexOf('_')) == base_code )
            {
                code = installed_code;
                found = true;
                break;
            }
        }
        if ( !found )
        {
            log::Log("Translations").log(
                QString("There is no translation for language %1 (%2)")
                .arg(language_name(code))
                .arg(code)
            );
            return;
        }
    }

    QCoreApplication* app = QCoreApplication::instance();
    app->removeTranslator(translator());
    current_language = code;
    app->installTranslator(translator());
}
