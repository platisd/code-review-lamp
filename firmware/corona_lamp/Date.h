#pragma once

namespace corona_lamp
{
struct Date
{
    Date(int y, int m, int d)
        : year{y}
        , month{m}
        , day{d}
    {
    }

    operator String() const
    {
        return String(month) + String("-") + String(day) + String("-") + String(year);
    }

    int year;
    int month;
    int day;
};
} // namespace corona_lamp
