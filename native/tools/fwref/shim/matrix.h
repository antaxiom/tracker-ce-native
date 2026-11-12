#ifndef FWREF_MATRIX_H_
#define FWREF_MATRIX_H_

#include <cmath>
#include <cstdint>

// Host-side EVE matrix stack (FT81x CMD_LOADIDENTITY/TRANSLATE/ROTATE/
// SETMATRIX). Coefficients are 16.16 fixed point; translation is accumulated
// in device-pixel 16.16 units. cLabel's rotate-around-centre idiom
// (translate(+c), rotate, translate(-c)) therefore has zero net offset.
struct FwrefMatrix
{
    int32_t a = 65536;
    int32_t b = 0;
    int32_t c = 0;
    int32_t d = 65536;
    int32_t tx = 0;
    int32_t ty = 0;
    bool active = false;

    void loadIdentity()
    {
        a = 65536;
        b = 0;
        c = 0;
        d = 65536;
        tx = 0;
        ty = 0;
        active = false;
    }

    void translate(int32_t x, int32_t y)
    {
        tx += x;
        ty += y;
    }

    void rotate(int32_t angleRaw)
    {
        constexpr double kPi = 3.14159265358979323846;
        const int32_t angle = angleRaw & 0xFFFF;
        const double radians = (static_cast<double>(angle) * 2.0 * kPi) / 65536.0;
        const int32_t ca = static_cast<int32_t>(std::llround(std::cos(radians) * 65536.0));
        const int32_t sa = static_cast<int32_t>(std::llround(std::sin(radians) * 65536.0));
        const int64_t A = a, B = b, C = c, D = d;
        a = static_cast<int32_t>((A * ca + C * sa) >> 16);
        b = static_cast<int32_t>((B * ca + D * sa) >> 16);
        c = static_cast<int32_t>((C * ca - A * sa) >> 16);
        d = static_cast<int32_t>((D * ca - B * sa) >> 16);
    }

    void setMatrix() { active = true; }

    static bool near(int32_t value, int32_t expected)
    {
        return std::abs(value - expected) <= 64;
    }

    // 0/1/2/3 for a pure quarter-turn, or -1 for anything else.
    int quarterTurns() const
    {
        if (near(a, 65536) && near(b, 0) && near(c, 0) && near(d, 65536))
            return 0;
        if (near(a, 0) && near(b, 65536) && near(c, -65536) && near(d, 0))
            return 1;
        if (near(a, -65536) && near(b, 0) && near(c, 0) && near(d, -65536))
            return 2;
        if (near(a, 0) && near(b, -65536) && near(c, 65536) && near(d, 0))
            return 3;
        return -1;
    }
};

#endif // FWREF_MATRIX_H_
