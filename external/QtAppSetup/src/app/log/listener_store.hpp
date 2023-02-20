/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "app/log/logger.hpp"

namespace app::log {

class ListenerStore: public LogListener
{
public:
    const std::vector<LogLine>& lines() const
    {
        return lines_;
    }

protected:
    void on_line(const LogLine& line) override
    {
        lines_.push_back(line);
    }

private:
    std::vector<LogLine> lines_;
};

} // namespace app::log

