/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <tuple>

namespace glaxnimate::model::detail {


template<class FuncT, class... Args, std::size_t... I>
auto invoke_impl(const FuncT& fun, std::index_sequence<I...>, const std::tuple<Args...>& args)
{
  return fun(std::get<I>(args)...);
}

template<int ArgCount, class FuncT, class... Args>
auto invoke(const FuncT& fun, const Args&... t)
{
  return invoke_impl(fun, std::make_index_sequence<ArgCount>(), std::make_tuple(t...));
}

} // namespace glaxnimate::model::detail
