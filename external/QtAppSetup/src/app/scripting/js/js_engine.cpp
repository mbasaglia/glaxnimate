/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "js_engine.hpp"


app::scripting::ScriptEngine::Autoregister<app::scripting::js::JsEngine> app::scripting::js::JsEngine::autoreg;
