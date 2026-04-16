#include <span>

#include "PiSubmarine/Max17261/Device.h"

namespace PiSubmarine::Max17261
{
    namespace
    {
        constexpr uint16_t PercentRegisterFullScale = 100u * 256u;

        constexpr NormalizedIntFactor DecodePercentRegister(uint16_t raw)
        {
            constexpr uint64_t maxFactorRaw = NormalizedIntFactor::GetMaxRawValue();
            const uint64_t scaled = (static_cast<uint64_t>(raw) * maxFactorRaw + (PercentRegisterFullScale / 2u)) /
                PercentRegisterFullScale;
            return NormalizedIntFactor{scaled};
        }

        constexpr uint16_t EncodePercentRegister(NormalizedIntFactor factor)
        {
            constexpr uint64_t maxFactorRaw = NormalizedIntFactor::GetMaxRawValue();
            const uint64_t raw = (factor.Get() * PercentRegisterFullScale + (maxFactorRaw / 2u)) / maxFactorRaw;
            return static_cast<uint16_t>(raw);
        }

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

	Result<void> Device::Init(const WaitFunc& waitFunc, MicroAmpereHours designCapacity, MicroAmperes terminationCurrent,
		MicroVolts emptyVoltage, bool forceReset)
	{
		if (forceReset)
		{
			if (auto result = ExecuteCommand(Command::Reset); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			waitFunc(std::chrono::milliseconds(500));

			auto config2Result = ReadRegister<uint16_t>(RegOffset::Config2);
			if (!config2Result.has_value())
			{
				return std::unexpected(config2Result.error());
			}
			auto config2 = config2Result.value();
			config2 |= 0x0001;
			if (auto result = WriteRegister(RegOffset::Config2, config2); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			waitFunc(std::chrono::milliseconds(500));
		}

		auto statusResult = ReadRegister<Status>(RegOffset::Status);
		if (!statusResult.has_value())
		{
			return std::unexpected(statusResult.error());
		}
		auto status = statusResult.value();

		if (RegUtils::HasAllFlags(status, Status::PowerOnReset))
		{
			while (true)
			{
				auto fstatResult = ReadRegister<FuelGaugeStatus>(RegOffset::FStat);
				if (!fstatResult.has_value())
				{
					return std::unexpected(fstatResult.error());
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
				return std::unexpected(hibCfgResult.error());
			}
			auto hibCfg = hibCfgResult.value();

			if (auto result = ExecuteCommand(Command::SoftWakeup); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			if (auto result = WriteRegister<uint16_t>(RegOffset::HibCfg, 0); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			if (auto result = ExecuteCommand(Command::Clear); !result.has_value())
			{
				return std::unexpected(result.error());
			}

			if (auto result = SetDesignCapacity(designCapacity); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			if (auto result = SetChargeTerminationCurrent(terminationCurrent); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			if (auto result = SetBatteryEmptyVoltage(emptyVoltage); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			if (auto result = SetBatteryChemistryModel(ModelId::Li); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			if (auto result = SetHighVoltageChargeProfileEnabled(false); !result.has_value())
			{
				return std::unexpected(result.error());
			}
			if (auto result = SetModelRefreshRequested(true); !result.has_value())
			{
				return std::unexpected(result.error());
			}

			while (true)
			{
				auto refreshFlagResult = IsModelRefreshInProgress();
				if (!refreshFlagResult.has_value())
				{
					return std::unexpected(refreshFlagResult.error());
				}
				if (!refreshFlagResult.value())
				{
					break;
				}
				waitFunc(std::chrono::milliseconds(10));
			}

			if (auto result = WriteRegister(RegOffset::HibCfg, hibCfg); !result.has_value())
			{
				return std::unexpected(result.error());
			}
		}

		while (true)
		{
			statusResult = ReadRegister<Status>(RegOffset::Status);
			if (!statusResult.has_value())
			{
				return std::unexpected(statusResult.error());
			}
			status = statusResult.value();

			if (!RegUtils::HasAnyFlag(status, Status::PowerOnReset))
			{
				break;
			}

			status = RegUtils::operator&(status, RegUtils::operator~(Status::PowerOnReset));
			if (auto result = SetAlertStatus(status); !result.has_value())
			{
				return std::unexpected(result.error());
			}
		}

		return {};
	}

    Result<Status> Device::GetAlertStatus() const
    {
        return ReadRegister<Status>(RegOffset::Status);
    }

    Result<void> Device::SetAlertStatus(Status value)
    {
        return WriteRegister(RegOffset::Status, value);
    }

    Result<FuelGaugeStatus> Device::GetFuelGaugeStatus() const
    {
        return ReadRegister<FuelGaugeStatus>(RegOffset::FStat);
    }

    Result<uint8_t> Device::GetHibernateTaskPeriodScale() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 0, 3);
    }

    Result<void> Device::SetHibernateTaskPeriodScale(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 0, 3, value);
    }

    Result<uint8_t> Device::GetHibernateExitDelay() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 3, 2);
    }

    Result<void> Device::SetHibernateExitDelay(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 3, 2, value);
    }

    Result<uint8_t> Device::GetHibernateCurrentThreshold() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 8, 4);
    }

    Result<void> Device::SetHibernateCurrentThreshold(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 8, 4, value);
    }

    Result<uint8_t> Device::GetHibernateEnterDelay() const
    {
        return ReadField<uint8_t>(RegOffset::HibCfg, 12, 3);
    }

    Result<void> Device::SetHibernateEnterDelay(uint8_t value)
    {
        return WriteField(RegOffset::HibCfg, 12, 3, value);
    }

    Result<bool> Device::IsHibernateModeEnabled() const
    {
        auto valueResult = ReadField<uint8_t>(RegOffset::HibCfg, 15, 1);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return valueResult.value() != 0;
    }

    Result<void> Device::SetHibernateModeEnabled(bool value)
    {
        return WriteField<uint8_t>(RegOffset::HibCfg, 15, 1, static_cast<uint8_t>(value));
    }

    Result<void> Device::ExecuteCommand(Command command)
    {
        return WriteRegister(RegOffset::Command, command);
    }

    Result<Command> Device::GetCommandRegisterValue() const
    {
        return ReadRegister<Command>(RegOffset::Command);
    }

    Result<void> Device::SetDesignCapacity(MicroAmpereHours valueMah)
    {
        uint16_t value = valueMah.ToRaw();
        return WriteRegister(RegOffset::DesignCap, value);
    }

    Result<MicroAmpereHours> Device::GetDesignCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::DesignCap);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<void> Device::SetChargeTerminationCurrent(MicroAmperes valueMa)
    {
        int16_t value = valueMa.ToRaw();
        return WriteRegister(RegOffset::IChgTerm, value);
    }

    Result<MicroAmperes> Device::GetChargeTerminationCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::IChgTerm);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmperes::FromRaw(valueResult.value());
    }

    Result<void> Device::SetBatteryEmptyVoltage(MicroVolts valueUv)
    {
        uint16_t value = valueUv.GetMicroVolts() / 10000;
        return WriteField(RegOffset::VEmpty, 7, 9, value);
    }

    Result<MicroVolts> Device::GetBatteryEmptyVoltage() const
    {
        auto valueResult = ReadField<uint16_t>(RegOffset::VEmpty, 7, 9);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroVolts(valueResult.value() * 10000);
    }

    Result<void> Device::SetBatteryRecoveryVoltage(MicroVolts valueUv)
    {
        uint16_t value = valueUv.GetMicroVolts() / 40000;
        return WriteField(RegOffset::VEmpty, 0, 7, value);
    }

    Result<MicroVolts> Device::GetBatteryRecoveryVoltage() const
    {
        auto valueResult = ReadField<uint16_t>(RegOffset::VEmpty, 0, 7);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroVolts(valueResult.value() * 40000);
    }

    Result<ModelId> Device::GetBatteryChemistryModel() const
    {
        return ReadField<ModelId>(RegOffset::ModelCfg, 4, 4);
    }

    Result<void> Device::SetBatteryChemistryModel(ModelId value)
    {
        return WriteField(RegOffset::ModelCfg, 4, 4, value);
    }

    Result<bool> Device::IsHighVoltageChargeProfileEnabled() const
    {
        auto valueResult = ReadField<uint8_t>(RegOffset::ModelCfg, 10, 1);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return valueResult.value() != 0;
    }

    Result<void> Device::SetHighVoltageChargeProfileEnabled(bool value)
    {
        return WriteField<uint8_t>(RegOffset::ModelCfg, 10, 1, static_cast<uint8_t>(value));
    }

    Result<bool> Device::IsModelRefreshInProgress() const
    {
        auto valueResult = ReadField<uint8_t>(RegOffset::ModelCfg, 15, 1);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return valueResult.value() != 0;
    }

    Result<void> Device::SetModelRefreshRequested(bool value)
    {
        return WriteField<uint8_t>(RegOffset::ModelCfg, 15, 1, static_cast<uint8_t>(value));
    }

    Result<MicroAmpereHours> Device::GetRemainingCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RepCap);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<MicroAmpereHours> Device::GetReportedFullCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FullCapRep);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<void> Device::SetReportedFullCapacityEstimate(MicroAmpereHours valueuAh)
    {
        uint16_t value = valueuAh.ToRaw();
        return WriteRegister(RegOffset::FullCapRep, value);
    }

    Result<MicroAmpereHours> Device::GetNominalFullCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FullCapNom);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<void> Device::SetNominalFullCapacityEstimate(MicroAmpereHours cap)
    {
        uint16_t value = cap.ToRaw();
        return WriteRegister(RegOffset::FullCapNom, value);
    }

    Result<NormalizedIntFactor> Device::GetRemainingStateOfCharge() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RepSOC);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return DecodePercentRegister(valueResult.value());
    }

    Result<MicroAmperes> Device::GetInstantCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::Current);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroAmperes::FromRaw(valueResult.value());
    }

    Result<uint16_t> Device::GetTimeToEmptyRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::TTE);
    }

    Result<uint16_t> Device::GetTimeToFullRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::TTF);
    }

    Result<uint16_t> Device::GetCycleCount() const
    {
        return ReadRegister<uint16_t>(RegOffset::Cycles);
    }

    Result<void> Device::SetCycleCount(uint16_t value)
    {
        return WriteRegister(RegOffset::Cycles, value);
    }

    Result<uint16_t> Device::GetTemperatureCompensationBaseline() const
    {
        return ReadRegister<uint16_t>(RegOffset::RComp0);
    }

    Result<void> Device::SetTemperatureCompensationBaseline(uint16_t value)
    {
        return WriteRegister(RegOffset::RComp0, value);
    }

    Result<uint16_t> Device::GetTemperatureCompensationCoefficient() const
    {
        return ReadRegister<uint16_t>(RegOffset::TempCo);
    }

    Result<void> Device::SetTemperatureCompensationCoefficient(uint16_t value)
    {
        return WriteRegister(RegOffset::TempCo, value);
    }

    Result<MicroVolts> Device::GetInstantVoltage() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VCell);
        if (!valueResult.has_value())
        {
            return std::unexpected(valueResult.error());
        }
        return MicroVolts::FromRaw(valueResult.value());
    }

    Result<ConfigFlags> Device::GetDeviceConfiguration() const
    {
        return ReadRegister<ConfigFlags>(RegOffset::Config);
    }

    Result<void> Device::SetDeviceConfiguration(ConfigFlags value)
    {
        return WriteRegister(RegOffset::Config, value);
    }

    Result<AlertThresholdRaw8> Device::GetVoltageAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    Result<void> Device::SetVoltageAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::VAlrtTh, EncodeAlertThreshold(value));
    }

    Result<AlertThresholdRaw8> Device::GetTemperatureAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    Result<void> Device::SetTemperatureAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::TAlrtTh, EncodeAlertThreshold(value));
    }

    Result<AlertThresholdRaw8> Device::GetSocAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::SAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    Result<void> Device::SetSocAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::SAlrtTh, EncodeAlertThreshold(value));
    }

    Result<AlertThresholdRaw8> Device::GetCurrentAlertThresholdRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::IAlrtTh);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeAlertThreshold(valueResult.value());
    }

    Result<void> Device::SetCurrentAlertThresholdRaw(const AlertThresholdRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::IAlrtTh, EncodeAlertThreshold(value));
    }

    Result<MicroAmperes> Device::GetTheoreticalLoadCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::AtRate);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmperes::FromRaw(valueResult.value());
    }

    Result<void> Device::SetTheoreticalLoadCurrent(MicroAmperes value)
    {
        return WriteRegister<int16_t>(RegOffset::AtRate, value.ToRaw());
    }

    Result<NormalizedIntFactor> Device::GetAgeEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::Age);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodePercentRegister(valueResult.value());
    }

    Result<void> Device::SetAgeEstimate(NormalizedIntFactor value)
    {
        return WriteRegister<uint16_t>(RegOffset::Age, EncodePercentRegister(value));
    }

    Result<MilliCelsius> Device::GetBatteryTemperature() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::Temp);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MilliCelsius::FromRaw(valueResult.value());
    }

    Result<void> Device::SetBatteryTemperature(MilliCelsius value)
    {
        return WriteRegister<int16_t>(RegOffset::Temp, value.ToRaw());
    }

    Result<MicroAmperes> Device::GetAverageCurrent() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::AvgCurrent);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmperes::FromRaw(valueResult.value());
    }

    Result<void> Device::SetAverageCurrent(MicroAmperes value)
    {
        return WriteRegister<int16_t>(RegOffset::AvgCurrent, value.ToRaw());
    }

    Result<CapacityAccumulator> Device::GetResidualCapacityTerm() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QResidual);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CapacityAccumulator{valueResult.value()};
    }

    Result<void> Device::SetResidualCapacityTerm(CapacityAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::QResidual, value.Raw);
    }

    Result<NormalizedIntFactor> Device::GetMixedSoc() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MixSOC);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodePercentRegister(valueResult.value());
    }

    Result<void> Device::SetMixedSoc(NormalizedIntFactor value)
    {
        return WriteRegister<uint16_t>(RegOffset::MixSOC, EncodePercentRegister(value));
    }

    Result<NormalizedIntFactor> Device::GetFilteredSoc() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::AvSOC);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodePercentRegister(valueResult.value());
    }

    Result<void> Device::SetFilteredSoc(NormalizedIntFactor value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvSOC, EncodePercentRegister(value));
    }

    Result<MicroAmpereHours> Device::GetMixedCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MixCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<void> Device::SetMixedCapacityEstimate(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::MixCap, value.ToRaw());
    }

    Result<ModelQrParameter> Device::GetModelQrTable00() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable00);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    Result<void> Device::SetModelQrTable00(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable00, value.Raw);
    }

    Result<NormalizedIntFactor> Device::GetFullSocThreshold() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FullSocThr);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodePercentRegister(valueResult.value());
    }

    Result<void> Device::SetFullSocThreshold(NormalizedIntFactor value)
    {
        return WriteRegister<uint16_t>(RegOffset::FullSocThr, EncodePercentRegister(value));
    }

    Result<uint16_t> Device::GetCellResistanceRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::RCell);
    }

    Result<void> Device::SetCellResistanceRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::RCell, value);
    }

    Result<MilliCelsius> Device::GetAverageTemperature() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::AvgTA);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MilliCelsius::FromRaw(valueResult.value());
    }

    Result<void> Device::SetAverageTemperature(MilliCelsius value)
    {
        return WriteRegister<int16_t>(RegOffset::AvgTA, value.ToRaw());
    }

    Result<MicroVolts> Device::GetAverageVoltage() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::AvgVCell);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroVolts::FromRaw(valueResult.value());
    }

    Result<void> Device::SetAverageVoltage(MicroVolts value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvgVCell, value.ToRaw());
    }

    Result<MinMaxRaw8> Device::GetTemperatureMinMaxRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MaxMinTemp);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeMinMax(valueResult.value());
    }

    Result<void> Device::SetTemperatureMinMaxRaw(const MinMaxRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxMinTemp, EncodeMinMax(value));
    }

    Result<MinMaxRaw8> Device::GetVoltageMinMaxRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MaxMinVolt);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeMinMax(valueResult.value());
    }

    Result<void> Device::SetVoltageMinMaxRaw(const MinMaxRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxMinVolt, EncodeMinMax(value));
    }

    Result<MinMaxRaw8> Device::GetCurrentMinMaxRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MaxMinCurr);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodeMinMax(valueResult.value());
    }

    Result<void> Device::SetCurrentMinMaxRaw(const MinMaxRaw8& value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxMinCurr, EncodeMinMax(value));
    }

    Result<MicroAmpereHours> Device::GetFilteredAvailableCapacityEstimate() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::AvCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<void> Device::SetFilteredAvailableCapacityEstimate(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvCap, value.ToRaw());
    }

    Result<uint16_t> Device::GetDeviceNameCode() const
    {
        return ReadRegister<uint16_t>(RegOffset::DevName);
    }

    Result<ModelQrParameter> Device::GetModelQrTable10() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable10);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    Result<void> Device::SetModelQrTable10(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable10, value.Raw);
    }

    Result<uint16_t> Device::GetAuxInputRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AIN);
    }

    Result<void> Device::SetAuxInputRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AIN, value);
    }

    Result<LearnConfiguration> Device::GetLearnConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::LearnCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return LearnConfiguration{valueResult.value()};
    }

    Result<void> Device::SetLearnConfiguration(LearnConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::LearnCfg, value.Raw);
    }

    Result<FilterConfiguration> Device::GetFilterConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FilterCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return FilterConfiguration{valueResult.value()};
    }

    Result<void> Device::SetFilterConfiguration(FilterConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::FilterCfg, value.Raw);
    }

    Result<RelaxConfiguration> Device::GetRelaxConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RelaxCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return RelaxConfiguration{valueResult.value()};
    }

    Result<void> Device::SetRelaxConfiguration(RelaxConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::RelaxCfg, value.Raw);
    }

    Result<MiscConfiguration> Device::GetMiscConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::MiscCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MiscConfiguration{valueResult.value()};
    }

    Result<void> Device::SetMiscConfiguration(MiscConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::MiscCfg, value.Raw);
    }

    Result<TemperatureGain> Device::GetTemperatureGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return TemperatureGain{valueResult.value()};
    }

    Result<void> Device::SetTemperatureGain(TemperatureGain value)
    {
        return WriteRegister<uint16_t>(RegOffset::TGain, value.Raw);
    }

    Result<TemperatureOffset> Device::GetTemperatureOffset() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TOff);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return TemperatureOffset{valueResult.value()};
    }

    Result<void> Device::SetTemperatureOffset(TemperatureOffset value)
    {
        return WriteRegister<uint16_t>(RegOffset::TOff, value.Raw);
    }

    Result<CurrentGain> Device::GetCurrentGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CurrentGain{valueResult.value()};
    }

    Result<void> Device::SetCurrentGain(CurrentGain value)
    {
        return WriteRegister<uint16_t>(RegOffset::CGain, value.Raw);
    }

    Result<CurrentOffset> Device::GetCurrentOffset() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::COff);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CurrentOffset{valueResult.value()};
    }

    Result<void> Device::SetCurrentOffset(CurrentOffset value)
    {
        return WriteRegister<uint16_t>(RegOffset::COff, value.Raw);
    }

    Result<ModelQrParameter> Device::GetModelQrTable20() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable20);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    Result<void> Device::SetModelQrTable20(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable20, value.Raw);
    }

    Result<MilliCelsius> Device::GetDieTemperature() const
    {
        auto valueResult = ReadRegister<int16_t>(RegOffset::DieTemp);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MilliCelsius::FromRaw(valueResult.value());
    }

    Result<void> Device::SetDieTemperature(MilliCelsius value)
    {
        return WriteRegister<int16_t>(RegOffset::DieTemp, value.ToRaw());
    }

    Result<MicroAmpereHours> Device::GetLearnedFullCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::FullCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<void> Device::SetLearnedFullCapacity(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::FullCap, value.ToRaw());
    }

    Result<uint16_t> Device::GetTimerLowWord() const
    {
        return ReadRegister<uint16_t>(RegOffset::Timer);
    }

    Result<void> Device::SetTimerLowWord(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::Timer, value);
    }

    Result<uint16_t> Device::GetShutdownTimerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::ShdnTimer);
    }

    Result<void> Device::SetShutdownTimerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::ShdnTimer, value);
    }

    Result<ModelQrParameter> Device::GetModelQrTable30() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QRTable30);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ModelQrParameter{valueResult.value()};
    }

    Result<void> Device::SetModelQrTable30(ModelQrParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::QRTable30, value.Raw);
    }

    Result<ResistanceGain> Device::GetResistanceGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ResistanceGain{valueResult.value()};
    }

    Result<void> Device::SetResistanceGain(ResistanceGain value)
    {
        return WriteRegister<uint16_t>(RegOffset::RGain, value.Raw);
    }

    Result<CapacityAccumulator> Device::GetCapacityAccumulator() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::dQAcc);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CapacityAccumulator{valueResult.value()};
    }

    Result<void> Device::SetCapacityAccumulator(CapacityAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::dQAcc, value.Raw);
    }

    Result<PowerAccumulator> Device::GetPowerAccumulator() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::dPAcc);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return PowerAccumulator{valueResult.value()};
    }

    Result<void> Device::SetPowerAccumulator(PowerAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::dPAcc, value.Raw);
    }

    Result<ConvergenceConfiguration> Device::GetConvergenceConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::ConvgCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ConvergenceConfiguration{valueResult.value()};
    }

    Result<void> Device::SetConvergenceConfiguration(ConvergenceConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::ConvgCfg, value.Raw);
    }

    Result<MicroAmpereHours> Device::GetVoltageFilteredRemainingCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VFRemCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return MicroAmpereHours::FromRaw(valueResult.value());
    }

    Result<void> Device::SetVoltageFilteredRemainingCapacity(MicroAmpereHours value)
    {
        return WriteRegister<uint16_t>(RegOffset::VFRemCap, value.ToRaw());
    }

    Result<ChargeAccumulator> Device::GetChargeAccumulator() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::QH);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ChargeAccumulator{valueResult.value()};
    }

    Result<void> Device::SetChargeAccumulator(ChargeAccumulator value)
    {
        return WriteRegister<uint16_t>(RegOffset::QH, value.Raw);
    }

    Result<uint16_t> Device::GetStatus2Raw() const
    {
        return ReadRegister<uint16_t>(RegOffset::Status2);
    }

    Result<void> Device::SetStatus2Raw(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::Status2, value); }

    Result<uint16_t> Device::GetInstantPowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::Power);
    }

    Result<void> Device::SetInstantPowerRaw(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::Power, value); }

    Result<uint16_t> Device::GetDeviceIdOrUserMem2() const
    {
        return ReadRegister<uint16_t>(RegOffset::ID);
    }

    Result<void> Device::SetUserMem2(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::ID, value); }

    Result<uint16_t> Device::GetAveragePowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AvgPower);
    }

    Result<void> Device::SetAveragePowerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AvgPower, value);
    }

    Result<TimeToFullConfiguration> Device::GetTimeToFullConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::TTFCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return TimeToFullConfiguration{valueResult.value()};
    }

    Result<void> Device::SetTimeToFullConfiguration(TimeToFullConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::TTFCfg, value.Raw);
    }

    Result<CvMixedCapacityParameter> Device::GetCvMixedCapacity() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CVMixCap);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CvMixedCapacityParameter{valueResult.value()};
    }

    Result<void> Device::SetCvMixedCapacity(CvMixedCapacityParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::CVMixCap, value.Raw);
    }

    Result<CvHalfTimeParameter> Device::GetCvHalfTime() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CVHalfTime);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return CvHalfTimeParameter{valueResult.value()};
    }

    Result<void> Device::SetCvHalfTime(CvHalfTimeParameter value)
    {
        return WriteRegister<uint16_t>(RegOffset::CVHalfTime, value.Raw);
    }

    Result<ChargeGainTemperatureCoefficient> Device::GetChargeGainTemperatureCoefficient() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::CGTempCo);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return ChargeGainTemperatureCoefficient{valueResult.value()};
    }

    Result<void> Device::SetChargeGainTemperatureCoefficient(ChargeGainTemperatureCoefficient value)
    {
        return WriteRegister<uint16_t>(RegOffset::CGTempCo, value.Raw);
    }

    Result<PowerCurveConfiguration> Device::GetPowerCurveConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::Curve);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return PowerCurveConfiguration{valueResult.value()};
    }

    Result<void> Device::SetPowerCurveConfiguration(PowerCurveConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::Curve, value.Raw);
    }

    Result<uint16_t> Device::GetExtendedConfigurationRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::Config2);
    }

    Result<void> Device::SetExtendedConfigurationRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::Config2, value);
    }

    Result<VoltageRippleMeasurement> Device::GetVoltageRippleRaw() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VRipple);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return VoltageRippleMeasurement{valueResult.value()};
    }

    Result<void> Device::SetVoltageRippleRaw(VoltageRippleMeasurement value)
    {
        return WriteRegister<uint16_t>(RegOffset::VRipple, value.Raw);
    }

    Result<RippleConfiguration> Device::GetRippleConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::RippleCfg);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return RippleConfiguration{valueResult.value()};
    }

    Result<void> Device::SetRippleConfiguration(RippleConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::RippleCfg, value.Raw);
    }

    Result<uint16_t> Device::GetTimerHighWord() const
    {
        return ReadRegister<uint16_t>(RegOffset::TimerH);
    }

    Result<void> Device::SetTimerHighWord(uint16_t value) { return WriteRegister<uint16_t>(RegOffset::TimerH, value); }

    Result<uint16_t> Device::GetSenseResistorOrUserMem3Raw() const
    {
        return ReadRegister<uint16_t>(RegOffset::RSense);
    }

    Result<void> Device::SetSenseResistorOrUserMem3Raw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::RSense, value);
    }

    Result<SocOcvLimitConfiguration> Device::GetSocOcvLimitConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::ScOcvLim);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return SocOcvLimitConfiguration{valueResult.value()};
    }

    Result<void> Device::SetSocOcvLimitConfiguration(SocOcvLimitConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::ScOcvLim, value.Raw);
    }

    Result<VoltageGainCalibration> Device::GetVoltageGain() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::VGain);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return VoltageGainCalibration{valueResult.value()};
    }

    Result<void> Device::SetVoltageGain(VoltageGainCalibration value)
    {
        return WriteRegister<uint16_t>(RegOffset::VGain, value.Raw);
    }

    Result<SocHoldConfiguration> Device::GetSocHoldConfiguration() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::SOCHold);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return SocHoldConfiguration{valueResult.value()};
    }

    Result<void> Device::SetSocHoldConfiguration(SocHoldConfiguration value)
    {
        return WriteRegister<uint16_t>(RegOffset::SOCHold, value.Raw);
    }

    Result<uint16_t> Device::GetMaximumPeakPowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::MaxPeakPower);
    }

    Result<void> Device::SetMaximumPeakPowerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::MaxPeakPower, value);
    }

    Result<uint16_t> Device::GetSustainedPeakPowerRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::SusPeakPower);
    }

    Result<void> Device::SetSustainedPeakPowerRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::SusPeakPower, value);
    }

    Result<uint16_t> Device::GetPackResistanceRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::PackResistance);
    }

    Result<void> Device::SetPackResistanceRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::PackResistance, value);
    }

    Result<uint16_t> Device::GetSystemResistanceRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::SysResistance);
    }

    Result<void> Device::SetSystemResistanceRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::SysResistance, value);
    }

    Result<uint16_t> Device::GetMinimumSystemVoltageRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::MinSysVoltage);
    }

    Result<void> Device::SetMinimumSystemVoltageRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::MinSysVoltage, value);
    }

    Result<uint16_t> Device::GetMaximumPeakPowerCurrentRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::MPPCurrent);
    }

    Result<void> Device::SetMaximumPeakPowerCurrentRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::MPPCurrent, value);
    }

    Result<uint16_t> Device::GetSustainedPeakPowerCurrentRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::SPPCurrent);
    }

    Result<void> Device::SetSustainedPeakPowerCurrentRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::SPPCurrent, value);
    }

    Result<uint16_t> Device::GetAtRateResidualCapacityRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AtQResidual);
    }

    Result<void> Device::SetAtRateResidualCapacityRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtQResidual, value);
    }

    Result<uint16_t> Device::GetAtRateTimeToEmptyRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AtTTE);
    }

    Result<void> Device::SetAtRateTimeToEmptyRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtTTE, value);
    }

    Result<NormalizedIntFactor> Device::GetAtRateAverageSoc() const
    {
        auto valueResult = ReadRegister<uint16_t>(RegOffset::AtAvSOC);
        if (!valueResult.has_value()) return std::unexpected(valueResult.error());
        return DecodePercentRegister(valueResult.value());
    }

    Result<void> Device::SetAtRateAverageSoc(NormalizedIntFactor value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtAvSOC, EncodePercentRegister(value));
    }

    Result<uint16_t> Device::GetAtRateAverageCapacityRaw() const
    {
        return ReadRegister<uint16_t>(RegOffset::AtAvCap);
    }

    Result<void> Device::SetAtRateAverageCapacityRaw(uint16_t value)
    {
        return WriteRegister<uint16_t>(RegOffset::AtAvCap, value);
    }

    Result<AlgorithmLearningParameters> Device::GetAlgorithmLearningParameters() const
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

	Result<void> Device::SetAlgorithmLearningParameters(const AlgorithmLearningParameters& value)
	{
		if (auto result = SetLearnConfiguration(value.LearningConfig); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetFilterConfiguration(value.FilterConfig); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetRelaxConfiguration(value.RelaxConfig); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetMiscConfiguration(value.MiscConfig); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetTemperatureGain(value.TempGain); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetTemperatureOffset(value.TempOffset); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetCurrentGain(value.ChargeCurrentGain); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetCurrentOffset(value.ChargeCurrentOffset); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetTemperatureCompensationBaseline(value.TemperatureCompensationBaselineRaw); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetTemperatureCompensationCoefficient(value.TemperatureCompensationCoefficientRaw); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetCapacityAccumulator(value.CapacityDeltaAccumulator); !result.has_value()) return std::unexpected(result.error());
		if (auto result = SetPowerAccumulator(value.PowerDeltaAccumulator); !result.has_value()) return std::unexpected(result.error());
		return {};
	}

	Result<void> Device::ReadRegisterRaw(RegOffset reg, uint8_t outData[2]) const
	{
		auto offset = static_cast<uint8_t>(reg);
		return m_Driver.WriteRead(Address, std::span<const uint8_t>(&offset, 1), std::span<uint8_t>(outData, 2));
	}

	Result<void> Device::WriteRegisterRaw(RegOffset reg, const uint8_t data[2])
	{
		uint8_t txData[3];
		txData[0] = static_cast<uint8_t>(reg);
		txData[1] = data[0];
		txData[2] = data[1];
		return m_Driver.Write(Address, std::span<const uint8_t>(txData, sizeof(txData)));
	}
}

