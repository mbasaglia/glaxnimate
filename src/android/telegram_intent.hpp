#ifndef TELEGRAMINTENT_HPP
#define TELEGRAMINTENT_HPP

#include <QString>

class TelegramIntent
{
public:
    class Result
    {
    public:
        Result(const QString& message = {}) : message_(message) {}

        explicit operator bool() const
        {
            return message_.isEmpty();
        }

        const QString& message() const
        {
            return message_;
        }

    private:
        QString message_;
    };

    Result send_stickers(const QStringList& filenames, const QStringList& emoji);
};

#endif // TELEGRAMINTENT_HPP
