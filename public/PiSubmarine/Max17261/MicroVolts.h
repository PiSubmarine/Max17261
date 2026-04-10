#pragma once

#include <cstdint>
#include <limits>

#if __has_include("PiSubmarine/Volts.h")
#include "PiSubmarine/Volts.h"
#endif

namespace PiSubmarine::Max17261
{

    // MicroVolts struct with precise uV handling
    struct MicroVolts
    {
        constexpr static size_t BitShift = 1ULL;
        
        // Constructor
        constexpr explicit MicroVolts(uint64_t uV = 0) : value(uV << BitShift) {}

        constexpr uint64_t GetMicroVolts() const
        {
            uint64_t temp = value >> BitShift;
            return temp;
        }

#if __has_include("PiSubmarine/Volts.h")
        constexpr PiSubmarine::Volts ToVolts() const
        {
            return PiSubmarine::Volts{ static_cast<double>(GetMicroVolts()) / 1'000'000.0 };
        }
#endif

        constexpr uint32_t GetMilliVolts() const
        {
            return static_cast<uint32_t>(GetMicroVolts() / 1000);
        }

        // Convert from raw 16-bit device value (0.078125 mV per LSB)
        static constexpr MicroVolts FromRaw(uint16_t raw)
        {
            // 1 LSB = 78.125 uV = 6250 / 80
            uint64_t value = ((raw * 6250) << BitShift) / 80;
            MicroVolts result;
            result.value = value;
            return result;
        }

        // Convert back to raw device format
        constexpr uint16_t ToRaw() const
        {
            // Multiply by 80, divide by 6250 to get raw units (rounded)
            int64_t temp = ((value * 80) >> BitShift) + 3125; // rounding (6250 / 2)
            int64_t raw = temp / 6250;

            // Clamp to uint16_t
            if (raw > std::numeric_limits<uint16_t>::max())
            {
                return std::numeric_limits<uint16_t>::max();
            }

            return static_cast<uint16_t>(raw);
        }

        // Arithmetic operators
        constexpr MicroVolts operator+(const MicroVolts& other) const
        {
            MicroVolts result;
            result.value = this->value + other.value;
            return result;
        }

        constexpr MicroVolts operator-(const MicroVolts& other) const
        {
            MicroVolts result;
            result.value = this->value - other.value;
            return result;
        }

        constexpr MicroVolts& operator+=(const MicroVolts& other)
        {
            this->value += other.value;
            return *this;
        }

        constexpr MicroVolts& operator-=(const MicroVolts& other)
        {
            this->value -= other.value;
            return *this;
        }

    private:
        uint64_t value;

        friend constexpr MicroVolts operator"" _uV(long double uV);
    };

    constexpr MicroVolts operator"" _uV(unsigned long long uV)
    {
        return MicroVolts(static_cast<uint64_t>(uV));
    }

    constexpr MicroVolts operator"" _uV(long double uV)
    {
        uint64_t value = static_cast<uint64_t>(uV * (1 << MicroVolts::BitShift));
        MicroVolts result;
        result.value = value;
        return result;
    }

}
