#pragma once

#include <cstdint>
#include <limits>

#if __has_include("PiSubmarine/Amperes.h")
#include "PiSubmarine/Amperes.h"
#endif

namespace PiSubmarine::Max17261
{
    struct MicroAmperes
    {
        constexpr static size_t BitShift = 4;
        constexpr static int64_t RSenseMicroOhms = 10000;

        // Constructor
        constexpr explicit MicroAmperes(int64_t uA = 0) : value(uA << BitShift) {}

        constexpr int64_t GetMicroAmperes() const
        {
            return value >> BitShift;
        }

#if __has_include("PiSubmarine/Amperes.h")
        constexpr PiSubmarine::Amperes ToAmperes() const
        {
            return PiSubmarine::Amperes{ static_cast<double>(GetMicroAmperes()) / 1'000'000.0 };
        }
#endif

        static constexpr MicroAmperes FromRaw(int16_t raw)
        {
            int64_t uv = static_cast<int64_t>(raw) * 1562500; // in nV
            int64_t ua = (uv * (1 << BitShift)) / RSenseMicroOhms;
            MicroAmperes result;
            result.value = ua;
            return result;
        }

        constexpr int16_t ToRaw() const
        {
            constexpr int64_t scale = 1562500;
            int64_t uv = value * RSenseMicroOhms;
            int64_t raw = uv / scale;
            raw /= (1 << BitShift);
            return static_cast<int16_t>(raw);
        }

        constexpr MicroAmperes operator+(const MicroAmperes& other) const
        {
            MicroAmperes result;
            result.value = this->value + other.value;
            return result;
        }

        constexpr MicroAmperes operator-(const MicroAmperes& other) const
        {
            MicroAmperes result;
            result.value = this->value - other.value;
            return result;
        }

        constexpr MicroAmperes& operator+=(const MicroAmperes& other)
        {
            this->value += other.value;
            return *this;
        }

        constexpr MicroAmperes& operator-=(const MicroAmperes& other)
        {
            this->value -= other.value;
            return *this;
        }

    private:
        int64_t value;

        friend constexpr MicroAmperes operator"" _uA(long double uA);
    };

    // Literal operator for integer microamperes
    constexpr MicroAmperes operator"" _uA(unsigned long long uA)
    {
        return MicroAmperes(static_cast<int64_t>(uA));
    }

    constexpr MicroAmperes operator"" _uA(long double uA)
    {
        int64_t value = static_cast<int64_t>(uA * (1 << MicroAmperes::BitShift));
        MicroAmperes result;
        result.value = value;
        return result;
    }

}
