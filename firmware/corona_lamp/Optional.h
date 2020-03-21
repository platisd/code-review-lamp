#pragma once

#include <cassert>

namespace corona_lamp
{
struct UninitializedOptional
{
};

static const UninitializedOptional Nullopt;

template<typename T>
class Optional
{
public:
    Optional() = default;
    Optional(const UninitializedOptional&) {}

    Optional(T&& t)
        : valid{true}
        , optionalValue{t}
    {
    }

    T value() const
    {
        assert(valid);
        return optionalValue;
    }

    bool operator!() const
    {
        return !valid;
    }

    void operator=(const T&& t)
    {
        optionalValue = t;
        valid         = true;
    }

private:
    bool valid{false};
    T optionalValue;
};

} // namespace corona_lamp
