#pragma once

#include <QObject>
#include <QCoreApplication>

namespace app::utils {

class TranslatedString
{
public:
    // Invoke using QT_TRANSLATE_NOOP("Settings", "")
    TranslatedString(const char* input) : input(input) {}
    TranslatedString(const QString& already_translated) : already_translated(std::move(already_translated)) {}

    TranslatedString() = default;

    operator QString() const
    {
        if ( !input || input[0] == '\0' )
            return already_translated;
        return QCoreApplication::translate("Settings", input);
    };

private:
    const char* input = nullptr;
    QString already_translated;
};

} // namespace app::utils
