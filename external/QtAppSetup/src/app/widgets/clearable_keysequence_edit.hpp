/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CLEARABLEKEYSEQUENCEEDIT_H
#define CLEARABLEKEYSEQUENCEEDIT_H

#include <memory>
#include <QWidget>
#include <QKeySequence>

class ClearableKeysequenceEdit : public QWidget
{
    Q_OBJECT

public:
    ClearableKeysequenceEdit(QWidget* parent = nullptr);
    ~ClearableKeysequenceEdit();

    void set_key_sequence(const QKeySequence& ks);
    void set_default_key_sequence(const QKeySequence& ks);

    QKeySequence key_sequence() const;

protected:
    void changeEvent ( QEvent* e ) override;

private Q_SLOTS:
    void use_default();
    void use_nothing();

private:
    class Private;
    std::unique_ptr<Private> d;
};

#endif // CLEARABLEKEYSEQUENCEEDIT_H
