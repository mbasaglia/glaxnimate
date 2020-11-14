#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#include "widgets/timeline/keyframe_editor_widget.hpp"

class KeyframeEditorDialog : public QDialog
{
public:
    KeyframeEditorDialog(const model::KeyframeTransition& trans = {}, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        lay = new QVBoxLayout(this);
        setLayout(lay);
        editor = new KeyframeEditorWidget(this);
        set_transition(trans);
        lay->addWidget(editor);
        box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, this);
        lay->addWidget(box);
        connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    void set_transition(const model::KeyframeTransition& trans)
    {
        this->trans = trans;
        editor->set_target(&this->trans);
    }

    const model::KeyframeTransition& transition() const
    {
        return trans;
    }

private:
    QVBoxLayout* lay;
    KeyframeEditorWidget* editor;
    QDialogButtonBox* box;
    model::KeyframeTransition trans;
};
