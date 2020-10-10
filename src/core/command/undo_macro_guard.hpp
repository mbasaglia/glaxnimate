#pragma once

#include "model/document.hpp"

namespace command {


class UndoMacroGuard
{
public:
    UndoMacroGuard(const QString& name, model::Document* document, bool start_macro = true)
    : name(name), document(document)
    {
        if ( start_macro )
            start();
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

private:
    QString name;
    model::Document* document;
    bool end_macro = false;
};


} // namespace command
