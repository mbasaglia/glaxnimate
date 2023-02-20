/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HELP_DIALOG_HPP
#define HELP_DIALOG_HPP

#include "base_dialog.hpp"

namespace glaxnimate::android {

class HelpDialog : public BaseDialog
{
    Q_OBJECT
public:
    HelpDialog(QWidget* parent = nullptr);
};

} // namespace glaxnimate::android

#endif // HELP_DIALOG_HPP
