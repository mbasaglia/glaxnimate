#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#include "ui/widgets/timeline/keyframe_editor_widget.hpp"

class KeyframeEditorDialog : public QDialog
{
public:
    KeyframeEditorDialog(const model::KeyframeTransition* trans = nullptr, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        set_transition(trans);
        lay = new QVBoxLayout(this);
        setLayout(lay);
        editor = new KeyframeEditorWidget(this);
        lay->addWidget(editor);
        editor->set_target(&this->trans);
        box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, this);
        lay->addWidget(box);
        connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    void set_transition(const model::KeyframeTransition* trans)
    {
        this->trans.set_hold(trans->hold());
        this->trans.set_before_handle(trans->before_handle());
        this->trans.set_after_handle(trans->after_handle());
    }

    bool hold() const { return trans.hold(); }
    model::KeyframeTransition::Descriptive before() const { return trans.before(); }
    model::KeyframeTransition::Descriptive after() const { return trans.after(); }
    QPointF before_handle() const { return trans.before_handle(); }
    QPointF after_handle() const { return trans.after_handle(); }

private:
    QVBoxLayout* lay;
    KeyframeEditorWidget* editor;
    QDialogButtonBox* box;
    model::KeyframeTransition trans;
};
