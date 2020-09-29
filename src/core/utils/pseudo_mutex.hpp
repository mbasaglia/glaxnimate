#pragma once

#include <mutex>

namespace utils {

class PseudoMutex
{
public:
    bool try_lock() noexcept
    {
        if ( locked )
            return false;

        locked = true;
        return true;
    }

    void lock() noexcept
    {
        locked = true;
    }

    void unlock() noexcept
    {
        locked = false;
    }

    explicit operator bool() const noexcept
    {
        return locked;
    }

    std::unique_lock<PseudoMutex> get_lock()
    {
        return std::unique_lock<PseudoMutex>(*this, std::try_to_lock);
    }

private:
    bool locked = false;
};

} // namespace utils
