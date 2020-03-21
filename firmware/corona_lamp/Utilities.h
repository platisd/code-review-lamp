#pragma once

namespace corona_lamp
{
template<typename T>
constexpr T getMap(const T& valueToMap, const T& fromLow, const T& fromHigh, const T& toLow, const T& toHigh)
{
    return fromHigh == fromLow ? toLow : (valueToMap - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}
} // namespace corona_lamp
