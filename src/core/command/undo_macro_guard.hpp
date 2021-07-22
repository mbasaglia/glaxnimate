#pragma once

#include "model/document.hpp"

namespace glaxnimate::command {


class UndoMacroGuard
{
public:
    UndoMacroGuard() noexcept : document(nullptr) {};

    UndoMacroGuard(const QString& name, model::Document* document, bool start_macro = true)
    : name(name), document(document)
    {
        if ( start_macro )
            start();
    }

    UndoMacroGuard(const UndoMacroGuard&) = delete;
    UndoMacroGuard& operator=(const UndoMacroGuard&) = delete;

    UndoMacroGuard(UndoMacroGuard&& other) noexcept
        : name(std::move(other.name)), document(other.document), end_macro(other.end_macro)
    {
        other.document = nullptr;
        other.end_macro = false;
    }

    UndoMacroGuard& operator=(UndoMacroGuard&& other) noexcept
    {
        std::swap(name, other.name);
        std::swap(document, other.document);
        std::swap(end_macro, other.end_macro);
        return *this;
    }

    ~UndoMacroGuard()
    {
        finish();
    }

    void start()
    {
        if ( !end_macro )
        {
            end_macro = true;
            document->undo_stack().beginMacro(name);
        }
    }

    void finish()
    {
        if ( end_macro )
        {
            end_macro = false;
            document->undo_stack().endMacro();
        }
    }

    bool started() const noexcept
    {
        return end_macro;
    }

private:
    QString name;
    model::Document* document;
    bool end_macro = false;
};


} // namespace glaxnimate::command
