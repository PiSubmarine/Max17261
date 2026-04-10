#include "PiSubmarine/Max17261/Device.h"

namespace PiSubmarine::Max17261
{
    namespace
    {
        constexpr AlertThresholdRaw8 DecodeAlertThreshold(uint16_t raw)
        {
            return AlertThresholdRaw8{
                .Maximum = static_cast<uint8_t>((raw >> 8) & 0xFF),
                .Minimum = static_cast<uint8_t>(raw & 0xFF)
            };
        }

        constexpr uint16_t EncodeAlertThreshold(const AlertThresholdRaw8& value)
        {
            return static_cast<uint16_t>((static_cast<uint16_t>(value.Maximum) << 8) | value.Minimum);
        }

        constexpr MinMaxRaw8 DecodeMinMax(uint16_t raw)
        {
            return MinMaxRaw8{
                .Maximum = static_cast<uint8_t>((raw >> 8) & 0xFF),
                .Minimum = static_cast<uint8_t>(raw & 0xFF)
            };
        }

        constexpr uint16_t EncodeMinMax(const MinMaxRaw8& value)
        {
            return static_cast<uint16_t>((static_cast<uint16_t>(value.Maximum) << 8) | value.Minimum);
        }
    }

    Device::Device(PiSubmarine::I2C::Api::IDriver& driver) : m_Driver(driver)
    {
    }

    bool Device::Init(const WaitFunc& waitFunc, MicroAmpereHours designCapacity, MicroAmperes terminationCurrent,
                      MicroVolts emptyVoltage, bool forceReset)
    {
        if (forceReset)
        {
            if (ExecuteCommand(Command::Reset) != DeviceError::None)
            {
                return false;
            }
            waitFunc(std::chrono::milliseconds(500));

            auto config2Result = ReadRegister<uint16_t>(RegOffset::Config2);
            if (!config2Result.has_value())
            {
                return false;
            }
            auto config2 = config2Result.value();
            config2 |= 0x0001;
            if (WriteRegister(RegOffset::Config2, config2) != DeviceError::None)
            {
                return false;
            }
            waitFunc(std::chrono::milliseconds(500));
        }

        auto statusResult = ReadRegister<Status>(RegOffset::Status);
        if (!statusResult.has_value())
        {
            return false;
        }
        auto status = statusResult.value();

        if (RegUtils::HasAllFlags(status, Status::PowerOnReset))
        {
            while (true)
            {
                auto fstatResult = ReadRegister<FuelGaugeStatus>(RegOffset::FStat);
                if (!fstatResult.has_value())
                {
                    return false;
                }
                auto fstat = fstatResult.value();

                bool dnr = RegUtils::HasAllFlags(fstat, FuelGaugeStatus::DataNotReady);
                if (!dnr)
                {
                    break;
                }
                waitFunc(std::chrono::milliseconds(10));
            }

            auto hibCfgResult = ReadRegister<uint16_t>(RegOffset::HibCfg);
            if (!hibCfgResult.has_value())
            {
                return false;
            }
            auto hibCfg = hibCfgResult.value();

            if (ExecuteCommand(Command::SoftWakeup) != DeviceError::None)
            {
                return false;
            }
            if (WriteRegister<uint16_t>(RegOffset::HibCfg, 0) != DeviceError::None)
            {
                return false;
            }
            if (ExecuteCommand(Command::Clear) != DeviceError::None)
            {
                return false;
            }

            // EZ Config
            if (SetDesignCapacity(designCapacity) != DeviceError::None ||
                SetChargeTerminationCurrent(terminationCurrent) != DeviceError::None ||
                SetBatteryEmptyVoltage(emptyVoltage) != DeviceError::None ||
                SetBatteryChemistryModel(ModelId::Li) != DeviceError::None ||
                SetHighVoltageChargeProfileEnabled(false) != DeviceError::None ||
                SetModelRefreshRequested(true) != DeviceError::None)
            {
                return false;
            }

            while (true)
            {
                auto refreshFlagResult = IsModelRefreshInProgress();
                if (!refreshFlagResult.has_value())
                {
                    return false;
                }
                if (!refreshFlagResult.value())
                {
                    break;
                }
                waitFunc(std::chrono::milliseconds(10));
            }

            if (WriteRegister(RegOffset::HibCfg, hibCfg) != DeviceError::None)
            {
                return false;
            }
        }

        while (true)
        {
            statusResult = ReadRegister<Status>(RegOffset::Status);
            if (!statusResult.has_value())
            {
                return false;
            }
            status = statusResult.value();

            if (!RegUtils::HasAnyFlag(status, Status::PowerOnReset))
            {
                break;
            }

            status = RegUtils::operator&(status, RegUtils::operator~(Status::PowerOnReset));
            if (SetAlertStatus(status) != DeviceError::None)
            {
                return false;
            }
        }

        return true;
    }

    std::expected<Status, DeviceError> Device::GetAlertStatus() const
    {
        return ReadRegister<Status>(RegOffset::Status);
    }

    DeviceError Device::SetAlertStatus(Status value)
    {
        return WriteRegister(RegOffset::Status, value);
    }

    std::expected<FuelGaugeStatus, DeviceError> Device::GetFuelGaugeStatus() const
    {
        return ReadRegister<FuelGaugeStatus>(RegOffset::FStat);
    }

    std::expected<uint8_t, DeviceError> Device::GetHibernateTaskPeriodScale() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 0, 3);
    }

    DeviceError Device::SetHibernateTaskPeriodScale(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 0, 3, value);
    }

    std::expected<uint8_t, DeviceError> Device::GetHibernateExitDelay() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 3, 2);
    }

    DeviceError Device::SetHibernateExitDelay(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 3, 2, value);
    }

    std::expected<uint8_t, DeviceError> Device::GetHibernateCurrentThreshold() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 8, 4);
    }

    DeviceError Device::SetHibernateCurrentThreshold(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 8, 4, value);
    }

    std::expected<uint8_t, DeviceError> Device::GetHibernateEnterDelay() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 12, 3);
    }

    DeviceError Device::SetHibernateEnterDelay(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 12, 3, value);
    }

    std::expected<bool, DeviceError> Device::IsHibernateModeEnabled() const
    {
        auto valueResult = ReadField<uint8_t>(RegOffset::HibCfg, 15, 1);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return valueResult.value() != 0;
    }

    DeviceError Device::SetHibernateModeEnabled(bool value)
    {
        return WriteField<uint8_t>(RegOffset::HibCfg, 15, 1, static_cast<uint8_t>(value));
    }

    DeviceError Device::ExecuteCommand(Command command)
    {
        return WriteRegister(RegOffset::Command, command);
    }

    std::expected<Command, DeviceError> Device::GetCommandRegisterValue() const
    {
        return ReadRegister<Command>(RegOffset::Command);
    }

    DeviceError Device::SetDesignCapacity(MicroAmpereHours valueMah)
    {
        uint16_t value = valueMah.ToRaw();
        return WriteRegister(RegOffset::DesignCap, value);
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetDesignCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::DesignCap);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    DeviceError Device::SetChargeTerminationCurrent(MicroAmperes valueMa)
    {
        int16_t value = valueMa.ToRaw();
        return WriteRegister(RegOffset::IChgTerm, value);
    }

    std::expected<MicroAmperes, DeviceError> Device::GetChargeTerminationCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::IChgTerm);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmperes::FromRaw(valueResult.value());
    }

    DeviceError Device::SetBatteryEmptyVoltage(MicroVolts valueUv)
    {
        uint16_t value = valueUv.GetMicroVolts() / 10000;
        return WriteField(RegOffset::VEmpty, 7, 9, value);
    }

    std::expected<MicroVolts, DeviceError> Device::GetBatteryEmptyVoltage() const
    {
        auto valueResult = ReadField<uint16_t>(RegOffset::VEmpty, 7, 9);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroVolts(valueResult.value() * 10000);
    }

    DeviceError Device::SetBatteryRecoveryVoltage(MicroVolts valueUv)
    {
        uint16_t value = valueUv.GetMicroVolts() / 40000;
        return WriteField(RegOffset::VEmpty, 0, 7, value);
    }

    std::expected<MicroVolts, DeviceError> Device::GetBatteryRecoveryVoltage() const
    {
        auto valueResult = ReadField<uint16_t>(RegOffset::VEmpty, 0, 7);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroVolts(valueResult.value() * 40000);
    }

    std::expected<ModelId, DeviceError> Device::GetBatteryChemistryModel() const
    {
        return ReadField<ModelId>(RegOffset::ModelCfg, 4, 4);
    }

    DeviceError Device::SetBatteryChemistryModel(ModelId value)
    {
        return WriteField(RegOffset::ModelCfg, 4, 4, value);
    }

    std::expected<bool, DeviceError> Device::IsHighVoltageChargeProfileEnabled() const
    {
        auto valueResult = ReadField<uint8_t>(RegOffset::ModelCfg, 10, 1);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return valueResult.value() != 0;
    }

    DeviceError Device::SetHighVoltageChargeProfileEnabled(bool value)
    {
        return WriteField<uint8_t>(RegOffset::ModelCfg, 10, 1, static_cast<uint8_t>(value));
    }

    std::expected<bool, DeviceError> Device::IsModelRefreshInProgress() const
    {
        auto valueResult = ReadField<uint8_t>(RegOffset::ModelCfg, 15, 1);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return valueResult.value() != 0;
    }

    DeviceError Device::SetModelRefreshRequested(bool value)
    {
        return WriteField<uint8_t>(RegOffset::ModelCfg, 15, 1, static_cast<uint8_t>(value));
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetRemainingCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RepCap);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetReportedFullCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FullCapRep);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    DeviceError Device::SetReportedFullCapacityEstimate(MicroAmpereHours valueuAh)
    {
        uint16_t value = valueuAh.ToRaw();
        return WriteRegister(RegOffset::FullCapRep, value);
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetNominalFullCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FullCapNom);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    DeviceError Device::SetNominalFullCapacityEstimate(MicroAmpereHours cap)
    {
        uint16_t value = cap.ToRaw();
        return WriteRegister(RegOffset::FullCapNom, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetRemainingStateOfChargeRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::RepSOC);
    }

    std::expected<MicroAmperes, DeviceError> Device::GetInstantCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::Current);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmperes::FromRaw(valueResult.value());
    }

    std::expected<uint16_t, DeviceError> Device::GetTimeToEmptyRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::TTE);
    }

    std::expected<uint16_t, DeviceError> Device::GetTimeToFullRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::TTF);
    }

    std::expected<uint16_t, DeviceError> Device::GetCycleCount() const
    {
        return ReadRegister<uint16_t>(RegOffset::Cycles);
    }

    DeviceError Device::SetCycleCount(uint16_t value)
    {
        return WriteRegister(RegOffset::Cycles, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetTemperatureCompensationBaseline() const
    {
        return ReadRegister<uint16_t>(RegOffset::RComp0);
    }

    DeviceError Device::SetTemperatureCompensationBaseline(uint16_t value)
    {
        return WriteRegister(RegOffset::RComp0, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetTemperatureCompensationCoefficient() const
    {
        return ReadRegister<uint16_t>(RegOffset::TempCo);
    }

    DeviceError Device::SetTemperatureCompensationCoefficient(uint16_t value)
    {
        return WriteRegister(RegOffset::TempCo, value);
    }

    std::expected<MicroVolts, DeviceError> Device::GetInstantVoltage() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VCell);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroVolts::FromRaw(valueResult.value());
    }

    std::expected<ConfigFlags, DeviceError> Device::GetDeviceConfiguration() const
    {
        return ReadRegister<ConfigFlags>(RegOffset::Config);
    }

    DeviceError Device::SetDeviceConfiguration(ConfigFlags value)
    {
        return WriteRegister(RegOffset::Config, value);
    }

    std::expected<AlertThresholdRaw8, DeviceError> Device::GetVoltageAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    DeviceError Device::SetVoltageAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::VAlrtTh, EncodeAlertThreshold(value));
    }

    std::expected<AlertThresholdRaw8, DeviceError> Device::GetTemperatureAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    DeviceError Device::SetTemperatureAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::TAlrtTh, EncodeAlertThreshold(value));
    }

    std::expected<AlertThresholdRaw8, DeviceError> Device::GetSocAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::SAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    DeviceError Device::SetSocAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::SAlrtTh, EncodeAlertThreshold(value));
    }

    std::expected<AlertThresholdRaw8, DeviceError> Device::GetCurrentAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::IAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    DeviceError Device::SetCurrentAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::IAlrtTh, EncodeAlertThreshold(value));
    }

    std::expected<MicroAmperes, DeviceError> Device::GetTheoreticalLoadCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::AtRate);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmperes::FromRaw(valueResult.value());
    }

    DeviceError Device::SetTheoreticalLoadCurrent(MicroAmperes value)
    {
        return WriteRegister<int16_t>(RegOffset::AtRate, value.ToRaw());
    }

    std::expected<uint16_t, DeviceError> Device::GetAgeEstimateRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::Age);
    }

    DeviceError Device::SetAgeEstimateRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::Age, value);
    }

    std::expected<MilliCelsius, DeviceError> Device::GetBatteryTemperature() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::Temp);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MilliCelsius::FromRaw(valueResult.value());
    }

    DeviceError Device::SetBatteryTemperature(MilliCelsius value)
    {
        return WriteRegister<int16_t>(RegOffset::Temp, value.ToRaw());
    }

    std::expected<MicroAmperes, DeviceError> Device::GetAverageCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::AvgCurrent);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmperes::FromRaw(valueResult.value());
    }

    DeviceError Device::SetAverageCurrent(MicroAmperes value)
    {
        return WriteRegister<int16_t>(RegOffset::AvgCurrent, value.ToRaw());
    }

    std::expected<CapacityAccumulator, DeviceError> Device::GetResidualCapacityTerm() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QResidual);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CapacityAccumulator{valueResult.value()};
    }

    DeviceError Device::SetResidualCapacityTerm(CapacityAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::QResidual, value.Raw);
    }

    std::expected<uint16_t, DeviceError> Device::GetMixedSocRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::MixSOC);
    }

    DeviceError Device::SetMixedSocRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::MixSOC, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetFilteredSocRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AvSOC);
    }

    DeviceError Device::SetFilteredSocRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvSOC, value);
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetMixedCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MixCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    DeviceError Device::SetMixedCapacityEstimate(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::MixCap, value.ToRaw());
    }

    std::expected<ModelQrParameter, DeviceError> Device::GetModelQrTable00() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable00);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    DeviceError Device::SetModelQrTable00(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable00, value.Raw);
    }

    std::expected<uint16_t, DeviceError> Device::GetFullSocThresholdRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::FullSocThr);
    }

    DeviceError Device::SetFullSocThresholdRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::FullSocThr, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetCellResistanceRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::RCell);
    }

    DeviceError Device::SetCellResistanceRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::RCell, value);
    }

    std::expected<MilliCelsius, DeviceError> Device::GetAverageTemperature() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::AvgTA);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MilliCelsius::FromRaw(valueResult.value());
    }

    DeviceError Device::SetAverageTemperature(MilliCelsius value)
    {
        return WriteRegister<int16_t>(RegOffset::AvgTA, value.ToRaw());
    }

    std::expected<MicroVolts, DeviceError> Device::GetAverageVoltage() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::AvgVCell);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroVolts::FromRaw(valueResult.value());
    }

    DeviceError Device::SetAverageVoltage(MicroVolts value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvgVCell, value.ToRaw());
    }

    std::expected<MinMaxRaw8, DeviceError> Device::GetTemperatureMinMaxRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MaxMinTemp);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeMinMax(valueResult.value());
    }

    DeviceError Device::SetTemperatureMinMaxRaw(const MinMaxRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxMinTemp, EncodeMinMax(value));
    }

    std::expected<MinMaxRaw8, DeviceError> Device::GetVoltageMinMaxRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MaxMinVolt);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeMinMax(valueResult.value());
    }

    DeviceError Device::SetVoltageMinMaxRaw(const MinMaxRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxMinVolt, EncodeMinMax(value));
    }

    std::expected<MinMaxRaw8, DeviceError> Device::GetCurrentMinMaxRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MaxMinCurr);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeMinMax(valueResult.value());
    }

    DeviceError Device::SetCurrentMinMaxRaw(const MinMaxRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxMinCurr, EncodeMinMax(value));
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetFilteredAvailableCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::AvCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    DeviceError Device::SetFilteredAvailableCapacityEstimate(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvCap, value.ToRaw());
    }

    std::expected<uint16_t, DeviceError> Device::GetDeviceNameCode() const
    {
        return ReadRegister<uint16_t>(RegOffset::DevName);
    }

    std::expected<ModelQrParameter, DeviceError> Device::GetModelQrTable10() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable10);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    DeviceError Device::SetModelQrTable10(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable10, value.Raw);
    }

    std::expected<uint16_t, DeviceError> Device::GetAuxInputRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AIN);
    }

    DeviceError Device::SetAuxInputRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AIN, value);
    }

    std::expected<LearnConfiguration, DeviceError> Device::GetLearnConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::LearnCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return LearnConfiguration{valueResult.value()};
    }

    DeviceError Device::SetLearnConfiguration(LearnConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::LearnCfg, value.Raw);
    }

    std::expected<FilterConfiguration, DeviceError> Device::GetFilterConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FilterCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return FilterConfiguration{valueResult.value()};
    }

    DeviceError Device::SetFilterConfiguration(FilterConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::FilterCfg, value.Raw);
    }

    std::expected<RelaxConfiguration, DeviceError> Device::GetRelaxConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RelaxCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return RelaxConfiguration{valueResult.value()};
    }

    DeviceError Device::SetRelaxConfiguration(RelaxConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::RelaxCfg, value.Raw);
    }

    std::expected<MiscConfiguration, DeviceError> Device::GetMiscConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MiscCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MiscConfiguration{valueResult.value()};
    }

    DeviceError Device::SetMiscConfiguration(MiscConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::MiscCfg, value.Raw);
    }

    std::expected<TemperatureGain, DeviceError> Device::GetTemperatureGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return TemperatureGain{valueResult.value()};
    }

    DeviceError Device::SetTemperatureGain(TemperatureGain value)
    {
        return WriteRegister<uint16_t>(RegOffset::TGain, value.Raw);
    }

    std::expected<TemperatureOffset, DeviceError> Device::GetTemperatureOffset() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TOff);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return TemperatureOffset{valueResult.value()};
    }

    DeviceError Device::SetTemperatureOffset(TemperatureOffset value)
    {
        return WriteRegister<uint16_t>(RegOffset::TOff, value.Raw);
    }

    std::expected<CurrentGain, DeviceError> Device::GetCurrentGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CurrentGain{valueResult.value()};
    }

    DeviceError Device::SetCurrentGain(CurrentGain value)
    {
        return WriteRegister<uint16_t>(RegOffset::CGain, value.Raw);
    }

    std::expected<CurrentOffset, DeviceError> Device::GetCurrentOffset() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::COff);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CurrentOffset{valueResult.value()};
    }

    DeviceError Device::SetCurrentOffset(CurrentOffset value)
    {
        return WriteRegister<uint16_t>(RegOffset::COff, value.Raw);
    }

    std::expected<ModelQrParameter, DeviceError> Device::GetModelQrTable20() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable20);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    DeviceError Device::SetModelQrTable20(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable20, value.Raw);
    }

    std::expected<MilliCelsius, DeviceError> Device::GetDieTemperature() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::DieTemp);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MilliCelsius::FromRaw(valueResult.value());
    }

    DeviceError Device::SetDieTemperature(MilliCelsius value)
    {
        return WriteRegister<int16_t>(RegOffset::DieTemp, value.ToRaw());
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetLearnedFullCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FullCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    DeviceError Device::SetLearnedFullCapacity(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::FullCap, value.ToRaw());
    }

    std::expected<uint16_t, DeviceError> Device::GetTimerLowWord() const
    {
        return ReadRegister<uint16_t>(RegOffset::Timer);
    }

    DeviceError Device::SetTimerLowWord(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::Timer, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetShutdownTimerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::ShdnTimer);
    }

    DeviceError Device::SetShutdownTimerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::ShdnTimer, value);
    }

    std::expected<ModelQrParameter, DeviceError> Device::GetModelQrTable30() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable30);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    DeviceError Device::SetModelQrTable30(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable30, value.Raw);
    }

    std::expected<ResistanceGain, DeviceError> Device::GetResistanceGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ResistanceGain{valueResult.value()};
    }

    DeviceError Device::SetResistanceGain(ResistanceGain value)
    {
        return WriteRegister<uint16_t>(RegOffset::RGain, value.Raw);
    }

    std::expected<CapacityAccumulator, DeviceError> Device::GetCapacityAccumulator() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::dQAcc);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CapacityAccumulator{valueResult.value()};
    }

    DeviceError Device::SetCapacityAccumulator(CapacityAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::dQAcc, value.Raw);
    }

    std::expected<PowerAccumulator, DeviceError> Device::GetPowerAccumulator() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::dPAcc);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return PowerAccumulator{valueResult.value()};
    }

    DeviceError Device::SetPowerAccumulator(PowerAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::dPAcc, value.Raw);
    }

    std::expected<ConvergenceConfiguration, DeviceError> Device::GetConvergenceConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::ConvgCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ConvergenceConfiguration{valueResult.value()};
    }

    DeviceError Device::SetConvergenceConfiguration(ConvergenceConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::ConvgCfg, value.Raw);
    }

    std::expected<MicroAmpereHours, DeviceError> Device::GetVoltageFilteredRemainingCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VFRemCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    DeviceError Device::SetVoltageFilteredRemainingCapacity(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::VFRemCap, value.ToRaw());
    }

    std::expected<ChargeAccumulator, DeviceError> Device::GetChargeAccumulator() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QH);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ChargeAccumulator{valueResult.value()};
    }

    DeviceError Device::SetChargeAccumulator(ChargeAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::QH, value.Raw);
    }

    std::expected<uint16_t, DeviceError> Device::GetStatus2Raw() const
    {
        return ReadRegister<uint16_t>(RegOffset::Status2);
    }

    DeviceError Device::SetStatus2Raw(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::Status2, value); }

    std::expected<uint16_t, DeviceError> Device::GetInstantPowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::Power);
    }

    DeviceError Device::SetInstantPowerRaw(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::Power, value); }

    std::expected<uint16_t, DeviceError> Device::GetDeviceIdOrUserMem2() const
    {
        return ReadRegister<uint16_t>(RegOffset::ID);
    }

    DeviceError Device::SetUserMem2(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::ID, value); }

    std::expected<uint16_t, DeviceError> Device::GetAveragePowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AvgPower);
    }

    DeviceError Device::SetAveragePowerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvgPower, value);
    }

    std::expected<TimeToFullConfiguration, DeviceError> Device::GetTimeToFullConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TTFCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return TimeToFullConfiguration{valueResult.value()};
    }

    DeviceError Device::SetTimeToFullConfiguration(TimeToFullConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::TTFCfg, value.Raw);
    }

    std::expected<CvMixedCapacityParameter, DeviceError> Device::GetCvMixedCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CVMixCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CvMixedCapacityParameter{valueResult.value()};
    }

    DeviceError Device::SetCvMixedCapacity(CvMixedCapacityParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::CVMixCap, value.Raw);
    }

    std::expected<CvHalfTimeParameter, DeviceError> Device::GetCvHalfTime() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CVHalfTime);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CvHalfTimeParameter{valueResult.value()};
    }

    DeviceError Device::SetCvHalfTime(CvHalfTimeParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::CVHalfTime, value.Raw);
    }

    std::expected<ChargeGainTemperatureCoefficient, DeviceError> Device::GetChargeGainTemperatureCoefficient() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CGTempCo);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ChargeGainTemperatureCoefficient{valueResult.value()};
    }

    DeviceError Device::SetChargeGainTemperatureCoefficient(ChargeGainTemperatureCoefficient value)
    {
        return WriteRegister<uint16_t>(RegOffset::CGTempCo, value.Raw);
    }

    std::expected<PowerCurveConfiguration, DeviceError> Device::GetPowerCurveConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::Curve);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return PowerCurveConfiguration{valueResult.value()};
    }

    DeviceError Device::SetPowerCurveConfiguration(PowerCurveConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::Curve, value.Raw);
    }

    std::expected<uint16_t, DeviceError> Device::GetExtendedConfigurationRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::Config2);
    }

    DeviceError Device::SetExtendedConfigurationRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::Config2, value);
    }

    std::expected<VoltageRippleMeasurement, DeviceError> Device::GetVoltageRippleRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VRipple);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return VoltageRippleMeasurement{valueResult.value()};
    }

    DeviceError Device::SetVoltageRippleRaw(VoltageRippleMeasurement value)
    {
        return WriteRegister<uint16_t>(RegOffset::VRipple, value.Raw);
    }

    std::expected<RippleConfiguration, DeviceError> Device::GetRippleConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RippleCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return RippleConfiguration{valueResult.value()};
    }

    DeviceError Device::SetRippleConfiguration(RippleConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::RippleCfg, value.Raw);
    }

    std::expected<uint16_t, DeviceError> Device::GetTimerHighWord() const
    {
        return ReadRegister<uint16_t>(RegOffset::TimerH);
    }

    DeviceError Device::SetTimerHighWord(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::TimerH, value); }

    std::expected<uint16_t, DeviceError> Device::GetSenseResistorOrUserMem3Raw() const
    {
        return ReadRegister<uint16_t>(RegOffset::RSense);
    }

    DeviceError Device::SetSenseResistorOrUserMem3Raw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::RSense, value);
    }

    std::expected<SocOcvLimitConfiguration, DeviceError> Device::GetSocOcvLimitConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::ScOcvLim);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return SocOcvLimitConfiguration{valueResult.value()};
    }

    DeviceError Device::SetSocOcvLimitConfiguration(SocOcvLimitConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::ScOcvLim, value.Raw);
    }

    std::expected<VoltageGainCalibration, DeviceError> Device::GetVoltageGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return VoltageGainCalibration{valueResult.value()};
    }

    DeviceError Device::SetVoltageGain(VoltageGainCalibration value)
    {
        return WriteRegister<uint16_t>(RegOffset::VGain, value.Raw);
    }

    std::expected<SocHoldConfiguration, DeviceError> Device::GetSocHoldConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::SOCHold);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return SocHoldConfiguration{valueResult.value()};
    }

    DeviceError Device::SetSocHoldConfiguration(SocHoldConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::SOCHold, value.Raw);
    }

    std::expected<uint16_t, DeviceError> Device::GetMaximumPeakPowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::MaxPeakPower);
    }

    DeviceError Device::SetMaximumPeakPowerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxPeakPower, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetSustainedPeakPowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::SusPeakPower);
    }

    DeviceError Device::SetSustainedPeakPowerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::SusPeakPower, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetPackResistanceRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::PackResistance);
    }

    DeviceError Device::SetPackResistanceRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::PackResistance, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetSystemResistanceRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::SysResistance);
    }

    DeviceError Device::SetSystemResistanceRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::SysResistance, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetMinimumSystemVoltageRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::MinSysVoltage);
    }

    DeviceError Device::SetMinimumSystemVoltageRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::MinSysVoltage, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetMaximumPeakPowerCurrentRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::MPPCurrent);
    }

    DeviceError Device::SetMaximumPeakPowerCurrentRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::MPPCurrent, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetSustainedPeakPowerCurrentRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::SPPCurrent);
    }

    DeviceError Device::SetSustainedPeakPowerCurrentRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::SPPCurrent, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetAtRateResidualCapacityRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AtQResidual);
    }

    DeviceError Device::SetAtRateResidualCapacityRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtQResidual, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetAtRateTimeToEmptyRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AtTTE);
    }

    DeviceError Device::SetAtRateTimeToEmptyRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtTTE, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetAtRateAverageSocRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AtAvSOC);
    }

    DeviceError Device::SetAtRateAverageSocRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtAvSOC, value);
    }

    std::expected<uint16_t, DeviceError> Device::GetAtRateAverageCapacityRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AtAvCap);
    }

    DeviceError Device::SetAtRateAverageCapacityRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtAvCap, value);
    }

    std::expected<AlgorithmLearningParameters, DeviceError> Device::GetAlgorithmLearningParameters() const
    {
        AlgorithmLearningParameters p{};

        auto learnCfg = GetLearnConfiguration();
        if (!learnCfg.has_value()) return std::unexpected(learnCfg.error());
        p.LearningConfig = learnCfg.value();
        auto filterCfg = GetFilterConfiguration();
        if (!filterCfg.has_value()) return std::unexpected(filterCfg.error());
        p.FilterConfig = filterCfg.value();
        auto relaxCfg = GetRelaxConfiguration();
        if (!relaxCfg.has_value()) return std::unexpected(relaxCfg.error());
        p.RelaxConfig = relaxCfg.value();
        auto miscCfg = GetMiscConfiguration();
        if (!miscCfg.has_value()) return std::unexpected(miscCfg.error());
        p.MiscConfig = miscCfg.value();
        auto tGain = GetTemperatureGain();
        if (!tGain.has_value()) return std::unexpected(tGain.error());
        p.TempGain = tGain.value();
        auto tOff = GetTemperatureOffset();
        if (!tOff.has_value()) return std::unexpected(tOff.error());
        p.TempOffset = tOff.value();
        auto cGain = GetCurrentGain();
        if (!cGain.has_value()) return std::unexpected(cGain.error());
        p.ChargeCurrentGain = cGain.value();
        auto cOff = GetCurrentOffset();
        if (!cOff.has_value()) return std::unexpected(cOff.error());
        p.ChargeCurrentOffset = cOff.value();
        auto rComp0 = GetTemperatureCompensationBaseline();
        if (!rComp0.has_value()) return std::unexpected(rComp0.error());
        p.TemperatureCompensationBaselineRaw = rComp0.value();
        auto tempCo = GetTemperatureCompensationCoefficient();
        if (!tempCo.has_value()) return std::unexpected(tempCo.error());
        p.TemperatureCompensationCoefficientRaw = tempCo.value();
        auto dQAcc = GetCapacityAccumulator();
        if (!dQAcc.has_value()) return std::unexpected(dQAcc.error());
        p.CapacityDeltaAccumulator = dQAcc.value();
        auto dPAcc = GetPowerAccumulator();
        if (!dPAcc.has_value()) return std::unexpected(dPAcc.error());
        p.PowerDeltaAccumulator = dPAcc.value();
        return p;
    }

    DeviceError Device::SetAlgorithmLearningParameters(const AlgorithmLearningParameters& value)
    {
        DeviceError error = DeviceError::None;
        if (error = SetLearnConfiguration(value.LearningConfig); error != DeviceError::None) return error;
        if (error = SetFilterConfiguration(value.FilterConfig); error != DeviceError::None) return error;
        if (error = SetRelaxConfiguration(value.RelaxConfig); error != DeviceError::None) return error;
        if (error = SetMiscConfiguration(value.MiscConfig); error != DeviceError::None) return error;
        if (error = SetTemperatureGain(value.TempGain); error != DeviceError::None) return error;
        if (error = SetTemperatureOffset(value.TempOffset); error != DeviceError::None) return error;
        if (error = SetCurrentGain(value.ChargeCurrentGain); error != DeviceError::None) return error;
        if (error = SetCurrentOffset(value.ChargeCurrentOffset); error != DeviceError::None) return error;
        if (error = SetTemperatureCompensationBaseline(value.TemperatureCompensationBaselineRaw); error !=
            DeviceError::None) return error;
        if (error = SetTemperatureCompensationCoefficient(value.TemperatureCompensationCoefficientRaw); error !=
            DeviceError::None) return error;
        if (error = SetCapacityAccumulator(value.CapacityDeltaAccumulator); error != DeviceError::None) return error;
        if (error = SetPowerAccumulator(value.PowerDeltaAccumulator); error != DeviceError::None) return error;
        return DeviceError::None;
    }

    DeviceError Device::ReadRegisterRaw(RegOffset reg, uint8_t outData[2]) const
    {
        auto offset = static_cast<uint8_t>(reg);
        const bool ok = m_Driver.WriteRead(Address, &offset, 1, outData, 2);
        auto error = ok ? DeviceError::None : DeviceError::WriteReadFailed;
        return error;
    }

    DeviceError Device::WriteRegisterRaw(RegOffset reg, const uint8_t data[2])
    {
        uint8_t txData[3];
        txData[0] = static_cast<uint8_t>(reg);
        txData[1] = data[0];
        txData[2] = data[1];
        const bool ok = m_Driver.Write(Address, txData, sizeof(txData));
        auto error = ok ? DeviceError::None : DeviceError::WriteFailed;
        return error;
    }
}
