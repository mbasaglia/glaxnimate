/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QString>
#include <QMap>

class QTranslator;

namespace app {

class TranslationService
{
public:
    static TranslationService& instance()
    {
        static TranslationService instance;
        return instance;
    }

    void change_lang_code ( QString code );

    /**
     * \brief Initialize the resource system
     */
    void initialize(QString default_lang_code=QStringLiteral("en"));

    /**
     *  \brief Determine human readable language name from ISO 639-1 code
     *
     *  Depending on Qt version the returned string is either in English or
     *  in the language itself.
     *
     *  If lang_code is not rectognised, a null string is returned
    */
    QString language_name ( QString lang_code );


    /**
     *  \brief Register a translation
     *
     *  \param name Human-readable language name
     *  \param code ISO 639-1 language code
     *  \param file Path to the translation file, if empty no file gets loaded
    */
    void register_translation ( QString name, QString code, QString file );


    QString current_language_name();
    QString current_language_code();
    QTranslator* translator();

    /**
     *  \brief Map of language names to codes
     */
    const QMap<QString, QString>& available_languages();

private:
    TranslationService() = default;
    TranslationService(const TranslationService&) = delete;
    ~TranslationService() = default;


    QMap<QString, QString> lang_names; ///< map lang_name -> lang_code
    QMap<QString, QTranslator*> translators; ///< map lang_code -> translator
    QString current_language;


};

} // namespace app
