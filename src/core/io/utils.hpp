/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "model/animation/animatable.hpp"

namespace glaxnimate::io {

std::vector<std::unique_ptr<model::KeyframeBase>> split_keyframes(model::AnimatableBase* prop);

} // namespace glaxnimate::io
