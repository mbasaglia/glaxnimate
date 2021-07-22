#pragma once

#include <iterator>

#include <QByteArray>
#include <QDataStream>
#include <QSet>

#include "model/document.hpp"

namespace glaxnimate::gui::item_models {

class DragEncoder
{
public:
    void add_node(model::DocumentNode* n)
    {
        if ( !n )
            return;

        if ( !uniq.contains(n->uuid.get()) )
        {
            stream << n->uuid.get();
            uniq.insert(n->uuid.get());
        }
    }

    const QByteArray& data() const
    {
        return encoded;
    }

private:
    QByteArray encoded;
    QDataStream stream{&encoded, QIODevice::WriteOnly};
    QSet<QUuid> uniq;
};


template<class T = model::DocumentNode>
class DragDecoder
{
public:
    using value_type = T*;

    class iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = DragDecoder::value_type;
        using pointer = T*;
        using difference_type = int;
        using reference = T*;

        iterator& operator++()
        {
            if ( decoder )
            {
                if ( decoder->at_end() )
                {
                    decoder = nullptr;
                    ptr = nullptr;
                }
                else
                {
                    do
                        ptr = qobject_cast<T*>(decoder->next());
                    while ( !ptr );
                }
            }
            return *this;
        }

        value_type operator*() const noexcept
        {
            return ptr;
        }

        bool operator==(const iterator& other) const noexcept
        {
            return other.decoder == decoder;
        }

        bool operator!=(const iterator& other) const noexcept
        {
            return other.decoder != decoder;
        }

    private:
        friend DragDecoder;
        iterator(DragDecoder* decoder)
            : decoder(decoder)
        {
            if ( decoder )
                ++*this;

        }

        DragDecoder* decoder;
        value_type ptr = nullptr;
    };

    explicit DragDecoder(QByteArray encoded, model::Document* document)
        : encoded(std::move(encoded)),
          stream(&this->encoded, QIODevice::ReadOnly),
          document(document)
    {}

    T* next()
    {
        QUuid uuid;
        stream >> uuid;
        return qobject_cast<T*>(document->find_by_uuid(uuid));
    }

    bool at_end() const
    {
        return stream.atEnd();
    }

    iterator begin()
    {
        return iterator(this);
    }

    iterator end()
    {
        return iterator(nullptr);
    }

private:
    QByteArray encoded;
    QDataStream stream;
    model::Document* document;
};

} // namespace glaxnimate::gui::item_models
