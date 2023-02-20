/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#if __GNUC__ >= 4
#   define PY_HIDDEN  __attribute__ ((visibility ("hidden")))
#else
#   define PY_HIDDEN
#endif
