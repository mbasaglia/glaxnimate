#ifndef EMOJI_DATA_HPP
#define EMOJI_DATA_HPP

#include <vector>
#include <QString>

namespace glaxnimate::emoji {

struct Emoji
{
    QString name;
    QString unicode;
    QString hex_slug;
};

struct EmojiSubGroup
{
    QString name;
    std::vector<Emoji> emoji;
};

struct EmojiGroup
{
    QString name;
    std::vector<EmojiSubGroup> children;

    static const std::vector<EmojiGroup> table;

    const Emoji& first() const
    {
        return children[0].emoji[0];
    }
};

} // glaxnimate::emoji



#endif // EMOJI_DATA_HPP
