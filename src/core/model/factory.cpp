/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "factory.hpp"

glaxnimate::model::Object* glaxnimate::model::Factory::static_build(const QString& name, glaxnimate::model::Document* doc)
{
    return instance().build(name, doc);
}
