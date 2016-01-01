#ifndef LODTALK_MATH_HPP
#define LODTALK_MATH_HPP

#include "Lodtalk/ObjectModel.hpp"

namespace Lodtalk
{

inline SmallIntegerValue divideRoundNeg(SmallIntegerValue dividend, SmallIntegerValue divisor)
{
    // This algorithm was taken from the Squeak VM.
    assert(divisor != 0);
    if (dividend >= 0)
    {
        if (divisor > 0)
        {
            // Positive result.
            return dividend / divisor;
        }
        else
        {
            // Negative result. Round towards minus infinite.
            auto positiveDivisor = -divisor;
            return  -(dividend + positiveDivisor - 1) / divisor;
        }
    }
    else
    {
        auto positiveDividend = -dividend;
        if (divisor > 0)
        {
            // Negative result. Round towards minus infinite.
            return  -(positiveDividend + divisor - 1) / divisor;
        }
        else
        {
            // Positive result.
            return dividend / divisor;
        }
    }
}

inline SmallIntegerValue moduleRoundNeg(SmallIntegerValue dividend, SmallIntegerValue divisor)
{
    // This algorithm was taken from the Squeak VM.
    assert(divisor != 0);

    auto result = dividend % divisor;

    // Make sure the result has the same sign as the divisor.
    if (divisor < 0)
    {
        if (result > 0)
            result += divisor;
    }
    else
    {
        if (result < 0)
            result += divisor;
    }

    return result;
}

} // End of namespace Lodtalk

#endif //LODTALK_MATH_HPP
