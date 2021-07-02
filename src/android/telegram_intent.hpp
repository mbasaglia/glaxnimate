#ifndef TELEGRAMINTENT_HPP
#define TELEGRAMINTENT_HPP

#include <QString>

namespace glaxnimate::android {

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

} // namespace glaxnimate::android

#endif // TELEGRAMINTENT_HPP
