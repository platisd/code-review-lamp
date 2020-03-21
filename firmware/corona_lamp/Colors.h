#pragma once

namespace corona_lamp
{

struct RGBColor
{
    RGBColor(int r = 0, int g = 0, int b = 0)
        : red{r}
        , green{g}
        , blue{b}
    {
    }
    int red;
    int green;
    int blue;
};

struct HSVColor
{
    /**
      @param h    Hue ranged          [0,360)
      @param s    Saturation ranged   [0,100]
      @param v    Value ranged        [0,100]
    */
    HSVColor(int h = 0, int s = 0, int v = 0)
        : hue{h}
        , saturation{s}
        , value{v}
    {
    }
    int hue;
    int saturation;
    int value;

    /**
      Converts HSV colors to RGB that can be used for Neopixels
      so that we can adjust the brightness of the colors.
      Code adapted from: https://stackoverflow.com/a/14733008

      @param hsv  The color in HSV format to convert
      @return     The equivalent color in RGB
    */
    RGBColor toRGB() const
    {
        // Scale the HSV values to the expected range
        auto rangedHue = map(hue, 0, 359, 0, 255);
        auto rangedSat = map(saturation, 0, 100, 0, 255);
        auto rangedVal = map(value, 0, 100, 0, 255);

        if (rangedSat == 0)
        {
            return {rangedVal, rangedVal, rangedVal};
        }

        auto region    = rangedHue / 43;
        auto remainder = (rangedHue - (region * 43)) * 6;

        auto p = (rangedVal * (255 - rangedSat)) >> 8;
        auto q = (rangedVal * (255 - ((rangedSat * remainder) >> 8))) >> 8;
        auto t = (rangedVal * (255 - ((rangedSat * (255 - remainder)) >> 8))) >> 8;

        switch (region)
        {
        case 0:
            return {rangedVal, t, p};
        case 1:
            return {q, rangedVal, p};
        case 2:
            return {p, rangedVal, t};
        case 3:
            return {p, q, rangedVal};
        case 4:
            return {t, p, rangedVal};
        default:
            return {rangedVal, p, q};
        }
    }
};

} // namespace corona_lamp
