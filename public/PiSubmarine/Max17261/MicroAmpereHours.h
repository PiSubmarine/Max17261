#pragma once

#include <cstdint>
#include <limits>

#if __has_include("PiSubmarine/AmpereHours.h")
#include "PiSubmarine/AmpereHours.h"
#endif

namespace PiSubmarine::Max17261
{

    struct MicroAmpereHours
    {
        constexpr static size_t BitShift = 0;
        constexpr static size_t RSenseMicroOhms = 10000;

        constexpr uint64_t GetMicroAmpereHours() const
        {
            return value >> BitShift;
        }

#if __has_include("PiSubmarine/AmpereHours.h")
        constexpr PiSubmarine::AmpereHours ToAmpereHours() const
        {
            return PiSubmarine::AmpereHours{ static_cast<double>(GetMicroAmpereHours()) / 1'000'000.0 };
        }
#endif

        // Constructor
        constexpr explicit MicroAmpereHours(uint64_t uAh = 0) : value(uAh << BitShift) {}

        // Convert from raw device value to MicroAmpereHours
        static constexpr MicroAmpereHours FromRaw(uint16_t raw)
        {
            // 5.0 uVh per LSB = 5 uWh per Rsense (uOhm)
            // I = V / R -> uAh = raw * 5.0 / Rsense
            // Multiply raw by 5000 (to convert 5.0 to 5000 pAh)
            uint64_t temp = ((static_cast<uint64_t>(raw) * 5000000) << BitShift) / RSenseMicroOhms;

            MicroAmpereHours result;
            result.value = temp;
            return result;
        }

        // Convert to raw device format
        constexpr uint16_t ToRaw() const
        {
            // raw = uAh * Rsense / 5.0
            // Multiply by Rsense first, then divide by 5000 (with rounding)
            uint64_t temp = (value * RSenseMicroOhms) >> BitShift;
            uint64_t raw = temp / 5000000;

            return static_cast<uint16_t>(raw);
        }

        // Arithmetic operators
        constexpr MicroAmpereHours operator+(const MicroAmpereHours& other) const
        {
            MicroAmpereHours result;
            result.value = this->value + other.value;
            return result;
        }

        constexpr MicroAmpereHours operator-(const MicroAmpereHours& other) const
        {
            MicroAmpereHours result;
            result.value = this->value - other.value;
            return result;
        }

        constexpr MicroAmpereHours& operator+=(const MicroAmpereHours& other)
        {
            this->value += other.value;
            return *this;
        }

        constexpr MicroAmpereHours& operator-=(const MicroAmpereHours& other)
        {
            this->value -= other.value;
            return *this;
        }

    private:
        uint64_t value;

        friend constexpr MicroAmpereHours operator"" _uAh(long double uAh);
    };

    // Integer literal: 1000_uAh
    constexpr MicroAmpereHours operator"" _uAh(unsigned long long uAh)
    {
        return MicroAmpereHours(static_cast<uint64_t>(uAh));
    }

    // Floating-point literal: 123.456_uAh (PC-only use)
    constexpr MicroAmpereHours operator"" _uAh(long double uAh)
    {
        uint64_t value = static_cast<uint64_t>(uAh * (1 << MicroAmpereHours::BitShift));
        MicroAmpereHours result;
        result.value = value;
        return result;
    }

}
