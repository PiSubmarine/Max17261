#pragma once

#include "PiSubmarine/RegUtils.h"
#include "PiSubmarine/Max17261/MicroAmpereHours.h"
#include "PiSubmarine/Max17261/MicroAmperes.h"
#include "PiSubmarine/Max17261/MicroVolts.h"
#include "PiSubmarine/Max17261/MilliCelcius.h"
#include "PiSubmarine/I2C/Api/IDriver.h"
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <chrono>
#include <array>

namespace PiSubmarine::Max17261
{
	using WaitFunc = std::function<void(std::chrono::milliseconds)>;

	enum class RegOffset : uint8_t
	{
		Status = 0x00,         ///< Status and alert flags.
		VAlrtTh = 0x01,        ///< Voltage alert thresholds.
		TAlrtTh = 0x02,        ///< Temperature alert thresholds.
		SAlrtTh = 0x03,        ///< SOC alert thresholds.
		AtRate = 0x04,         ///< Theoretical load current for At* predictions.
		RepCap = 0x05,         ///< Reported remaining capacity.
		RepSOC = 0x06,         ///< Reported state-of-charge.
		Age = 0x07,            ///< Estimated battery age/capacity ratio.
		Temp = 0x08,           ///< Battery temperature.
		VCell = 0x09,          ///< Instantaneous battery voltage.
		Current = 0x0A,        ///< Instantaneous battery current.
		AvgCurrent = 0x0B,     ///< Averaged battery current.
		QResidual = 0x0C,      ///< Residual capacity term for ModelGauge.
		MixSOC = 0x0D,         ///< Mixed SOC estimator output.
		AvSOC = 0x0E,          ///< Filtered SOC estimator output.
		MixCap = 0x0F,         ///< Mixed remaining capacity estimator output.
		FullCapRep = 0x10,     ///< Reported full capacity estimate.
		TTE = 0x11,            ///< Time-to-empty estimate.
		QRTable00 = 0x12,      ///< Model QR table segment 0.
		FullSocThr = 0x13,     ///< Full SOC threshold.
		RCell = 0x14,          ///< Cell internal resistance estimate.
		AvgTA = 0x16,          ///< Averaged temperature.
		Cycles = 0x17,         ///< Cycle counter.
		DesignCap = 0x18,      ///< Design capacity configuration.
		AvgVCell = 0x19,       ///< Averaged battery voltage.
		MaxMinTemp = 0x1A,     ///< Maximum/minimum temperature history.
		MaxMinVolt = 0x1B,     ///< Maximum/minimum voltage history.
		MaxMinCurr = 0x1C,     ///< Maximum/minimum current history.
		Config = 0x1D,         ///< Main configuration and alert behavior.
		IChgTerm = 0x1E,       ///< Charge termination current.
		AvCap = 0x1F,          ///< Filtered available capacity estimate.
		TTF = 0x20,            ///< Time-to-full estimate.
		DevName = 0x21,        ///< Device identifier.
		QRTable10 = 0x22,      ///< Model QR table segment 1.
		FullCapNom = 0x23,     ///< Nominal full capacity.
		AIN = 0x27,            ///< Auxiliary ADC input measurement.
		LearnCfg = 0x28,       ///< Learning algorithm configuration.
		FilterCfg = 0x29,      ///< Filter configuration.
		RelaxCfg = 0x2A,       ///< Relaxation detection configuration.
		MiscCfg = 0x2B,        ///< Miscellaneous algorithm configuration.
		TGain = 0x2C,          ///< Temperature gain calibration.
		TOff = 0x2D,           ///< Temperature offset calibration.
		CGain = 0x2E,          ///< Current gain calibration.
		COff = 0x2F,           ///< Current offset calibration.
		QRTable20 = 0x32,      ///< Model QR table segment 2.
		DieTemp = 0x34,        ///< Internal die temperature.
		FullCap = 0x35,        ///< Learned full capacity.
		RComp0 = 0x38,         ///< Temperature compensation baseline.
		TempCo = 0x39,         ///< Temperature compensation slope.
		VEmpty = 0x3A,         ///< Empty/recovery voltage thresholds.
		FStat = 0x3D,          ///< Fuel-gauge algorithm status.
		Timer = 0x3E,          ///< Internal timer low word.
		ShdnTimer = 0x3F,      ///< Shutdown timer.
		QRTable30 = 0x42,      ///< Model QR table segment 3.
		RGain = 0x43,          ///< Resistance gain calibration.
		dQAcc = 0x45,          ///< Coulomb-count accumulation term.
		dPAcc = 0x46,          ///< Power accumulation term.
		ConvgCfg = 0x49,       ///< Convergence configuration.
		VFRemCap = 0x4A,       ///< Voltage-filtered remaining capacity.
		QH = 0x4D,             ///< Coulomb counter accumulator.
		Command = 0x60,        ///< Command register (reset/clear/wakeup).
		Status2 = 0xB0,        ///< Secondary status flags.
		Power = 0xB1,          ///< Instantaneous power.
		ID = 0xB2,             ///< Device ID (or UserMem2 alias).
		UserMem2 = 0xB2,       ///< Alias of ID for user memory access.
		AvgPower = 0xB3,       ///< Averaged power.
		IAlrtTh = 0xB4,        ///< Current alert thresholds.
		TTFCfg = 0xB5,         ///< Time-to-full configuration.
		CVMixCap = 0xB6,       ///< Constant-voltage mixed-capacity model term.
		CVHalfTime = 0xB7,     ///< Constant-voltage phase timing.
		CGTempCo = 0xB8,       ///< Charge-gain temperature coefficient.
		Curve = 0xB9,          ///< Dynamic-power curve configuration.
		HibCfg = 0xBA,         ///< Hibernate behavior configuration.
		Config2 = 0xBB,        ///< Secondary configuration.
		VRipple = 0xBC,        ///< Voltage ripple measurement.
		RippleCfg = 0xBD,      ///< Ripple filter/configuration.
		TimerH = 0xBE,         ///< Internal timer high word.
		RSense = 0xD0,         ///< Sense resistor value configuration (or UserMem3 alias).
		UserMem3 = 0xD0,       ///< Alias of RSense for user memory access.
		ScOcvLim = 0xD1,       ///< SOC/OCV convergence limits.
		VGain = 0xD2,          ///< Voltage gain calibration.
		SOCHold = 0xD3,        ///< SOC hold behavior configuration.
		MaxPeakPower = 0xD4,   ///< Maximum peak power estimate.
		SusPeakPower = 0xD5,   ///< Sustained peak power estimate.
		PackResistance = 0xD6, ///< Estimated pack resistance.
		SysResistance = 0xD7,  ///< Estimated system resistance.
		MinSysVoltage = 0xD8,  ///< Minimum system voltage constraint.
		MPPCurrent = 0xD9,     ///< Current at maximum peak power.
		SPPCurrent = 0xDA,     ///< Current at sustained peak power.
		ModelCfg = 0xDB,       ///< EZ model selection/refresh control.
		AtQResidual = 0xDC,    ///< AtRate residual capacity prediction.
		AtTTE = 0xDD,          ///< AtRate time-to-empty prediction.
		AtAvSOC = 0xDE,        ///< AtRate average SOC prediction.
		AtAvCap = 0xDF         ///< AtRate average capacity prediction.
	};

	enum class Status : uint16_t
	{
		PowerOnReset = (1 << 1),
		MinimumCurrentAlert = (1 << 2),
		BatteryPresent = (1 << 3),
		MaximumCurrentAlert = (1 << 6),
		StateOfChargeChanged = (1 << 7),
		MinimumVoltageAlert = (1 << 8),
		MinimumTemperatureAlert = (1 << 9),
		MinimumStateOfCharge = (1 << 10),
		MaximumVoltageAlert = (1 << 12),
		MaximumTemperatureAlert = (1 << 13),
		MaximumStateOfChargeAlert = (1 << 14),
		BatteryRemoved = (1 << 15)
	};

	enum class FuelGaugeStatus : uint16_t
	{
		DataNotReady = (1 << 0),
		RelaxedCellDetectionLong = (1 << 6),
		FullQualified = (1 << 7),
		EmptyDetection = (1 << 8),
		RelaxedCellDetectionShort = (1 << 9)
	};

	enum class Command : uint16_t
	{
		Clear = 0,
		Reset = 0x0F,
		SoftWakeup = 0x0090
	};

	enum class ModelId : uint8_t
	{
		Li = 0,
		NcrOrNca = 2,
		LiFePo = 6
	};

	enum class ConfigFlags : uint16_t
	{
		BatteryRemoved = (1 << 0),
		BatteryInserted = (1 << 1),
		AlertEnable = (1 << 2),
		ForceThermistorBias = (1 << 3),
		EnableThermistor = (1 << 4),
		CommShutdown = (1 << 6),
		Shutdown = (1 << 7),
		TemperatureExternal = (1 << 8),
		EnableTemperatureChannel = (1 << 9),
		ThPinShutdown = (1 << 10),
		CurrentAlertSticky = (1 << 11),
		VoltageAlertSticky = (1 << 12),
		TemperatureAlertSticky = (1 << 13),
		SocAlertSticky = (1 << 14),
		TSel = (1 << 15)
	};

	enum class DeviceError : uint8_t
	{
		None = 0,
		WriteFailed,
		WriteReadFailed
	};

	struct AlertThresholdRaw8
	{
		uint8_t Maximum = 0;
		uint8_t Minimum = 0;
	};

	struct MinMaxRaw8
	{
		uint8_t Maximum = 0;
		uint8_t Minimum = 0;
	};

	struct ModelQrParameter { uint16_t Raw = 0; };
	struct LearnConfiguration { uint16_t Raw = 0; };
	struct FilterConfiguration { uint16_t Raw = 0; };
	struct RelaxConfiguration { uint16_t Raw = 0; };
	struct MiscConfiguration { uint16_t Raw = 0; };
	struct TemperatureGain { uint16_t Raw = 0; };
	struct TemperatureOffset { uint16_t Raw = 0; };
	struct CurrentGain { uint16_t Raw = 0; };
	struct CurrentOffset { uint16_t Raw = 0; };
	struct ResistanceGain { uint16_t Raw = 0; };
	struct CapacityAccumulator { uint16_t Raw = 0; };
	struct PowerAccumulator { uint16_t Raw = 0; };
	struct ConvergenceConfiguration { uint16_t Raw = 0; };
	struct ChargeAccumulator { uint16_t Raw = 0; };
	struct TimeToFullConfiguration { uint16_t Raw = 0; };
	struct CvMixedCapacityParameter { uint16_t Raw = 0; };
	struct CvHalfTimeParameter { uint16_t Raw = 0; };
	struct ChargeGainTemperatureCoefficient { uint16_t Raw = 0; };
	struct PowerCurveConfiguration { uint16_t Raw = 0; };
	struct VoltageRippleMeasurement { uint16_t Raw = 0; };
	struct RippleConfiguration { uint16_t Raw = 0; };
	struct SocOcvLimitConfiguration { uint16_t Raw = 0; };
	struct VoltageGainCalibration { uint16_t Raw = 0; };
	struct SocHoldConfiguration { uint16_t Raw = 0; };

	struct AlgorithmLearningParameters
	{
		LearnConfiguration LearningConfig{};
		FilterConfiguration FilterConfig{};
		RelaxConfiguration RelaxConfig{};
		MiscConfiguration MiscConfig{};
		TemperatureGain TempGain{};
		TemperatureOffset TempOffset{};
		CurrentGain ChargeCurrentGain{};
		CurrentOffset ChargeCurrentOffset{};
		uint16_t TemperatureCompensationBaselineRaw = 0;
		uint16_t TemperatureCompensationCoefficientRaw = 0;
		CapacityAccumulator CapacityDeltaAccumulator{};
		PowerAccumulator PowerDeltaAccumulator{};

		constexpr static size_t SerializedSize = 24;

		constexpr std::array<uint8_t, SerializedSize> Serialize() const
		{
			std::array<uint8_t, SerializedSize> data{};
			const auto writeU16 = [&data](size_t index, uint16_t value) constexpr
			{
				data[index] = static_cast<uint8_t>(value & 0xFF);
				data[index + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
			};

			writeU16(0, LearningConfig.Raw);
			writeU16(2, FilterConfig.Raw);
			writeU16(4, RelaxConfig.Raw);
			writeU16(6, MiscConfig.Raw);
			writeU16(8, TempGain.Raw);
			writeU16(10, TempOffset.Raw);
			writeU16(12, ChargeCurrentGain.Raw);
			writeU16(14, ChargeCurrentOffset.Raw);
			writeU16(16, TemperatureCompensationBaselineRaw);
			writeU16(18, TemperatureCompensationCoefficientRaw);
			writeU16(20, CapacityDeltaAccumulator.Raw);
			writeU16(22, PowerDeltaAccumulator.Raw);
			return data;
		}

		static constexpr AlgorithmLearningParameters Deserialize(const std::array<uint8_t, SerializedSize>& data)
		{
			const auto readU16 = [&data](size_t index) constexpr -> uint16_t
			{
				return static_cast<uint16_t>(data[index]) |
					(static_cast<uint16_t>(data[index + 1]) << 8);
			};

			AlgorithmLearningParameters result{};
			result.LearningConfig.Raw = readU16(0);
			result.FilterConfig.Raw = readU16(2);
			result.RelaxConfig.Raw = readU16(4);
			result.MiscConfig.Raw = readU16(6);
			result.TempGain.Raw = readU16(8);
			result.TempOffset.Raw = readU16(10);
			result.ChargeCurrentGain.Raw = readU16(12);
			result.ChargeCurrentOffset.Raw = readU16(14);
			result.TemperatureCompensationBaselineRaw = readU16(16);
			result.TemperatureCompensationCoefficientRaw = readU16(18);
			result.CapacityDeltaAccumulator.Raw = readU16(20);
			result.PowerDeltaAccumulator.Raw = readU16(22);
			return result;
		}
	};

	class Device
	{
	public:
		constexpr static uint8_t Address = 0x36;

		/// <summary>
		/// Constructs a MAX17261 device wrapper using the provided blocking I2C driver.
		/// </summary>
		Device(I2C::Api::IDriver& driver);

		/// <summary>
		/// Initializes MAX1726 in blocking mode. Must be called on every power cycle.
		/// </summary>
		/// <returns>True if initialization was successfull.</returns>
		bool Init(const WaitFunc& waitFunc, MicroAmpereHours designCapacity, MicroAmperes terminationCurrent, MicroVolts emptyVoltage, bool forceReset = false);

		/// <summary>
		/// Reads Status register (00h) alert/status flags.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<Status, DeviceError> GetAlertStatus() const;
		/// <summary>
		/// Writes Status register (00h), typically to clear sticky alert bits.
		/// </summary>
		DeviceError SetAlertStatus(Status value);
		/// <summary>
		/// Reads FuelGaugeStatus register (3Dh) fuel-gauge algorithm status.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<FuelGaugeStatus, DeviceError> GetFuelGaugeStatus() const;
		/// <summary>
		/// Reads HibCfg.HibScalar (BAh[2:0]) hibernate task period scale.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint8_t, DeviceError> GetHibernateTaskPeriodScale() const;

		/// <summary>
		/// Sets HibCfg.HibScalar (BAh[2:0]).
		/// Task period is approximately 351ms * 2^HibScalar.
		/// </summary>
		DeviceError SetHibernateTaskPeriodScale(uint8_t value);

		/// <summary>
		/// Reads HibCfg.HibExitTime (BAh[4:3]) hibernate exit delay field.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint8_t, DeviceError> GetHibernateExitDelay() const;

		/// <summary>
		/// Sets HibCfg.HibExitTime (BAh[4:3]).
		/// Exit delay is approximately (HibExitTime + 1) * 702ms * 2^HibScalar.
		/// </summary>
		DeviceError SetHibernateExitDelay(uint8_t value);

		/// <summary>
		/// Reads HibCfg.HibThreshold (BAh[11:8]) current threshold for hibernate logic.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint8_t, DeviceError> GetHibernateCurrentThreshold() const;

		/// <summary>
		/// Sets HibCfg.HibThreshold (BAh[11:8]) current threshold for hibernate logic.
		/// </summary>
		DeviceError SetHibernateCurrentThreshold(uint8_t value);

		/// <summary>
		/// Reads HibCfg.HibEnterTime (BAh[14:12]) hibernate entry delay.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint8_t, DeviceError> GetHibernateEnterDelay() const;

		/// <summary>
		/// Sets HibCfg.HibEnterTime (BAh[14:12]) hibernate entry delay.
		/// </summary>
		DeviceError SetHibernateEnterDelay(uint8_t value);

		/// <summary>
		/// Reads HibCfg.EnHib (BAh[15]) hibernate enable state.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<bool, DeviceError> IsHibernateModeEnabled() const;

		/// <summary>
		/// Sets HibCfg.EnHib (BAh[15]) hibernate enable state.
		/// </summary>
		DeviceError SetHibernateModeEnabled(bool value);

		/// <summary>
		/// Writes Command register (60h) with control command (Clear/Reset/SoftWakeup).
		/// </summary>
		DeviceError ExecuteCommand(Command command);

		/// <summary>
		/// Reads Command register (60h).
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<Command, DeviceError> GetCommandRegisterValue() const;

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Writes DesignCap register (18h), design capacity.
		DeviceError SetDesignCapacity(MicroAmpereHours valueMah);

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Reads DesignCap register (18h), design capacity.
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetDesignCapacity() const;

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Writes IChgTerm register (1Eh), charge termination current.
		DeviceError SetChargeTerminationCurrent(MicroAmperes valueMa);

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Reads IChgTerm register (1Eh), charge termination current.
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmperes, DeviceError> GetChargeTerminationCurrent() const;

		/// <summary>
		/// Writes VEmpty.VEmpty (3Ah[15:7]) battery empty voltage threshold.
		/// </summary>
		DeviceError SetBatteryEmptyVoltage(MicroVolts valueUv);

		/// <summary>
		/// Reads VEmpty.VEmpty (3Ah[15:7]) battery empty voltage threshold.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroVolts, DeviceError> GetBatteryEmptyVoltage() const;

		/// <summary>
		/// Writes VEmpty.VRecovery (3Ah[6:0]) battery recovery voltage threshold.
		/// </summary>
		DeviceError SetBatteryRecoveryVoltage(MicroVolts valueUv);

		/// <summary>
		/// Reads VEmpty.VRecovery (3Ah[6:0]) battery recovery voltage threshold.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroVolts, DeviceError> GetBatteryRecoveryVoltage() const;

		/// <summary>
		/// Reads ModelCfg.ModelID (DBh[7:4]) battery chemistry model selection.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<ModelId, DeviceError> GetBatteryChemistryModel() const;

		/// <summary>
		/// Writes ModelCfg.ModelID (DBh[7:4]) battery chemistry model selection.
		/// </summary>
		DeviceError SetBatteryChemistryModel(ModelId value);

		/// <summary>
		/// Reads ModelCfg.VChg (DBh[10]) high-voltage charge profile selection.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<bool, DeviceError> IsHighVoltageChargeProfileEnabled() const;

		/// <summary>
		/// Writes ModelCfg.VChg (DBh[10]) high-voltage charge profile selection.
		/// </summary>
		DeviceError SetHighVoltageChargeProfileEnabled(bool value);

		/// <summary>
		/// Reads ModelCfg.Refresh (DBh[15]) model refresh in-progress flag.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<bool, DeviceError> IsModelRefreshInProgress() const;

		/// <summary>
		/// Writes ModelCfg.Refresh (DBh[15]) to request model refresh.
		/// </summary>
		DeviceError SetModelRefreshRequested(bool value);

		/// <summary>
		/// Reads RepCap register (05h), reported remaining capacity estimate.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetRemainingCapacityEstimate() const;

		/// <summary>
		/// Reads FullCapRep register (10h), reported full capacity estimate.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetReportedFullCapacityEstimate() const;

		/// <summary>
		/// Writes FullCapRep register (10h), reported full capacity estimate.
		/// </summary>
		DeviceError SetReportedFullCapacityEstimate(MicroAmpereHours valueuAh);

		/// <summary>
		/// Reads FullCapNom register (23h), nominal full capacity estimate.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetNominalFullCapacityEstimate() const;

		/// <summary>
		/// Writes FullCapNom register (23h), nominal full capacity estimate.
		/// </summary>
		DeviceError SetNominalFullCapacityEstimate(MicroAmpereHours cap);

		/// <summary>
		/// Reads RepSOC register (06h) in raw register units.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetRemainingStateOfChargeRaw() const;

		/// <summary>
		/// Reads Current register (0Ah), instantaneous pack current.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmperes, DeviceError> GetInstantCurrent() const;

		/// <summary>
		/// Reads TTE register (11h), time-to-empty in raw register units.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetTimeToEmptyRaw() const;

		/// <summary>
		/// Reads TTF register (20h), time-to-full in raw register units.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetTimeToFullRaw() const;

		/// <summary>
		/// Reads Cycles register (17h), cycle counter.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetCycleCount() const;

		/// <summary>
		/// Writes Cycles register (17h), cycle counter.
		/// </summary>
		DeviceError SetCycleCount(uint16_t value);

		/// <summary>
		/// Reads RComp0 register (38h), temperature compensation baseline value.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetTemperatureCompensationBaseline() const;

		/// <summary>
		/// Writes RComp0 register (38h), temperature compensation baseline value.
		/// </summary>
		DeviceError SetTemperatureCompensationBaseline(uint16_t value);

		/// <summary>
		/// Reads TempCo register (39h), temperature compensation coefficient.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetTemperatureCompensationCoefficient() const;

		/// <summary>
		/// Writes TempCo register (39h), temperature compensation coefficient.
		/// </summary>
		DeviceError SetTemperatureCompensationCoefficient(uint16_t value);

		/// <summary>
		/// Reads VCell register (09h), instantaneous cell/pack voltage.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroVolts, DeviceError> GetInstantVoltage() const;

		/// <summary>
		/// Reads Config register (1Dh), operation and alert-configuration flags.
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<ConfigFlags, DeviceError> GetDeviceConfiguration() const;

		/// <summary>
		/// Writes Config register (1Dh), operation and alert-configuration flags.
		/// </summary>
		DeviceError SetDeviceConfiguration(ConfigFlags value);

		/// <summary>
		/// Reads VAlrtTh register (01h) voltage alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<AlertThresholdRaw8, DeviceError> GetVoltageAlertThresholdRaw() const;
		/// <summary>
		/// Writes VAlrtTh register (01h) voltage alert thresholds (raw encoded bytes).
		/// </summary>
		DeviceError SetVoltageAlertThresholdRaw(const AlertThresholdRaw8& value);
		/// <summary>
		/// Reads TAlrtTh register (02h) temperature alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<AlertThresholdRaw8, DeviceError> GetTemperatureAlertThresholdRaw() const;
		/// <summary>
		/// Writes TAlrtTh register (02h) temperature alert thresholds (raw encoded bytes).
		/// </summary>
		DeviceError SetTemperatureAlertThresholdRaw(const AlertThresholdRaw8& value);
		/// <summary>
		/// Reads SAlrtTh register (03h) SOC alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<AlertThresholdRaw8, DeviceError> GetSocAlertThresholdRaw() const;
		/// <summary>
		/// Writes SAlrtTh register (03h) SOC alert thresholds (raw encoded bytes).
		/// </summary>
		DeviceError SetSocAlertThresholdRaw(const AlertThresholdRaw8& value);
		/// <summary>
		/// Reads IAlrtTh register (B4h) current alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] [[nodiscard]] std::expected<AlertThresholdRaw8, DeviceError> GetCurrentAlertThresholdRaw() const;
		/// <summary>
		/// Writes IAlrtTh register (B4h) current alert thresholds (raw encoded bytes).
		/// </summary>
		DeviceError SetCurrentAlertThresholdRaw(const AlertThresholdRaw8& value);

		/// <summary>Reads AtRate register (04h), theoretical load current input.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmperes, DeviceError> GetTheoreticalLoadCurrent() const;
		/// <summary>Writes AtRate register (04h), theoretical load current input.</summary>
		DeviceError SetTheoreticalLoadCurrent(MicroAmperes value);

		/// <summary>Reads Age register (07h) battery aging estimate in raw register format.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetAgeEstimateRaw() const;
		/// <summary>Writes Age register (07h) battery aging estimate in raw register format.</summary>
		DeviceError SetAgeEstimateRaw(uint16_t value);
		/// <summary>Reads Temp register (08h) measured battery temperature.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<MilliCelsius, DeviceError> GetBatteryTemperature() const;
		/// <summary>Writes Temp register (08h) measured battery temperature register value.</summary>
		DeviceError SetBatteryTemperature(MilliCelsius value);
		/// <summary>Reads AvgCurrent register (0Bh), averaged battery current.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmperes, DeviceError> GetAverageCurrent() const;
		/// <summary>Writes AvgCurrent register (0Bh), averaged battery current register value.</summary>
		DeviceError SetAverageCurrent(MicroAmperes value);
		/// <summary>Reads QResidual register (0Ch), internal residual capacity term. Internal register.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<CapacityAccumulator, DeviceError> GetResidualCapacityTerm() const;
		/// <summary>Writes QResidual register (0Ch). Internal register.</summary>
		DeviceError SetResidualCapacityTerm(CapacityAccumulator value);
		/// <summary>Reads MixSOC register (0Dh), mixed SOC estimate in raw format.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetMixedSocRaw() const;
		/// <summary>Writes MixSOC register (0Dh), mixed SOC estimate in raw format.</summary>
		DeviceError SetMixedSocRaw(uint16_t value);
		/// <summary>Reads AvSOC register (0Eh), filtered SOC estimate in raw format.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetFilteredSocRaw() const;
		/// <summary>Writes AvSOC register (0Eh), filtered SOC estimate in raw format.</summary>
		DeviceError SetFilteredSocRaw(uint16_t value);
		/// <summary>Reads MixCap register (0Fh), mixed capacity estimate.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetMixedCapacityEstimate() const;
		/// <summary>Writes MixCap register (0Fh), mixed capacity estimate.</summary>
		DeviceError SetMixedCapacityEstimate(MicroAmpereHours value);
		/// <summary>Reads QRTable00 register (12h). Internal model register.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<ModelQrParameter, DeviceError> GetModelQrTable00() const;
		/// <summary>Writes QRTable00 register (12h). Internal model register.</summary>
		DeviceError SetModelQrTable00(ModelQrParameter value);
		/// <summary>Reads FullSocThr register (13h) full SOC threshold in raw format.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetFullSocThresholdRaw() const;
		/// <summary>Writes FullSocThr register (13h) full SOC threshold in raw format.</summary>
		DeviceError SetFullSocThresholdRaw(uint16_t value);
		/// <summary>Reads RCell register (14h), internal resistance estimate in raw format.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<uint16_t, DeviceError> GetCellResistanceRaw() const;
		/// <summary>Writes RCell register (14h), internal resistance estimate in raw format.</summary>
		DeviceError SetCellResistanceRaw(uint16_t value);
		/// <summary>Reads AvgTA register (16h), averaged temperature.</summary>
		[[nodiscard]] [[nodiscard]] std::expected<MilliCelsius, DeviceError> GetAverageTemperature() const;
		/// <summary>Writes AvgTA register (16h), averaged temperature register value.</summary>
		DeviceError SetAverageTemperature(MilliCelsius value);
		/// <summary>Reads AvgVCell register (19h), averaged voltage.</summary>
		[[nodiscard]] std::expected<MicroVolts, DeviceError> GetAverageVoltage() const;
		/// <summary>Writes AvgVCell register (19h), averaged voltage register value.</summary>
		DeviceError SetAverageVoltage(MicroVolts value);
		/// <summary>Reads MaxMinTemp register (1Ah), max/min temperature bytes in one transaction.</summary>
		[[nodiscard]] std::expected<MinMaxRaw8, DeviceError> GetTemperatureMinMaxRaw() const;
		/// <summary>Writes MaxMinTemp register (1Ah), max/min temperature bytes in one transaction.</summary>
		DeviceError SetTemperatureMinMaxRaw(const MinMaxRaw8& value);
		/// <summary>Reads MaxMinVolt register (1Bh), max/min voltage bytes in one transaction.</summary>
		[[nodiscard]] std::expected<MinMaxRaw8, DeviceError> GetVoltageMinMaxRaw() const;
		/// <summary>Writes MaxMinVolt register (1Bh), max/min voltage bytes in one transaction.</summary>
		DeviceError SetVoltageMinMaxRaw(const MinMaxRaw8& value);
		/// <summary>Reads MaxMinCurr register (1Ch), max/min current bytes in one transaction.</summary>
		[[nodiscard]] std::expected<MinMaxRaw8, DeviceError> GetCurrentMinMaxRaw() const;
		/// <summary>Writes MaxMinCurr register (1Ch), max/min current bytes in one transaction.</summary>
		DeviceError SetCurrentMinMaxRaw(const MinMaxRaw8& value);
		/// <summary>Reads AvCap register (1Fh), filtered available capacity estimate.</summary>
		[[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetFilteredAvailableCapacityEstimate() const;
		/// <summary>Writes AvCap register (1Fh), filtered available capacity estimate.</summary>
		DeviceError SetFilteredAvailableCapacityEstimate(MicroAmpereHours value);
		/// <summary>Reads DevName register (21h) silicon device identifier.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetDeviceNameCode() const;
		/// <summary>Reads QRTable10 register (22h). Internal model register.</summary>
		[[nodiscard]] std::expected<ModelQrParameter, DeviceError> GetModelQrTable10() const;
		/// <summary>Writes QRTable10 register (22h). Internal model register.</summary>
		DeviceError SetModelQrTable10(ModelQrParameter value);
		/// <summary>Reads AIN register (27h), auxiliary ADC input in raw format.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetAuxInputRaw() const;
		/// <summary>Writes AIN register (27h), auxiliary ADC input register value.</summary>
		DeviceError SetAuxInputRaw(uint16_t value);
		/// <summary>Reads LearnCfg register (28h). Internal learning register.</summary>
		[[nodiscard]] std::expected<LearnConfiguration, DeviceError> GetLearnConfiguration() const;
		/// <summary>Writes LearnCfg register (28h). Internal learning register.</summary>
		DeviceError SetLearnConfiguration(LearnConfiguration value);
		/// <summary>Reads FilterCfg register (29h). Internal filter register.</summary>
		[[nodiscard]] std::expected<FilterConfiguration, DeviceError> GetFilterConfiguration() const;
		/// <summary>Writes FilterCfg register (29h). Internal filter register.</summary>
		DeviceError SetFilterConfiguration(FilterConfiguration value);
		/// <summary>Reads RelaxCfg register (2Ah). Internal relax register.</summary>
		[[nodiscard]] std::expected<RelaxConfiguration, DeviceError> GetRelaxConfiguration() const;
		/// <summary>Writes RelaxCfg register (2Ah). Internal relax register.</summary>
		DeviceError SetRelaxConfiguration(RelaxConfiguration value);
		/// <summary>Reads MiscCfg register (2Bh). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<MiscConfiguration, DeviceError> GetMiscConfiguration() const;
		/// <summary>Writes MiscCfg register (2Bh). Internal algorithm register.</summary>
		DeviceError SetMiscConfiguration(MiscConfiguration value);
		/// <summary>Reads TGain register (2Ch). Internal calibration register.</summary>
		[[nodiscard]] std::expected<TemperatureGain, DeviceError> GetTemperatureGain() const;
		/// <summary>Writes TGain register (2Ch). Internal calibration register.</summary>
		DeviceError SetTemperatureGain(TemperatureGain value);
		/// <summary>Reads TOff register (2Dh). Internal calibration register.</summary>
		[[nodiscard]] std::expected<TemperatureOffset, DeviceError> GetTemperatureOffset() const;
		/// <summary>Writes TOff register (2Dh). Internal calibration register.</summary>
		DeviceError SetTemperatureOffset(TemperatureOffset value);
		/// <summary>Reads CGain register (2Eh). Internal calibration register.</summary>
		[[nodiscard]] std::expected<CurrentGain, DeviceError> GetCurrentGain() const;
		/// <summary>Writes CGain register (2Eh). Internal calibration register.</summary>
		DeviceError SetCurrentGain(CurrentGain value);
		/// <summary>Reads COff register (2Fh). Internal calibration register.</summary>
		[[nodiscard]] std::expected<CurrentOffset, DeviceError> GetCurrentOffset() const;
		/// <summary>Writes COff register (2Fh). Internal calibration register.</summary>
		DeviceError SetCurrentOffset(CurrentOffset value);
		/// <summary>Reads QRTable20 register (32h). Internal model register.</summary>
		[[nodiscard]] std::expected<ModelQrParameter, DeviceError> GetModelQrTable20() const;
		/// <summary>Writes QRTable20 register (32h). Internal model register.</summary>
		DeviceError SetModelQrTable20(ModelQrParameter value);
		/// <summary>Reads DieTemp register (34h), internal IC die temperature.</summary>
		[[nodiscard]] std::expected<MilliCelsius, DeviceError> GetDieTemperature() const;
		/// <summary>Writes DieTemp register (34h), internal IC die temperature register value.</summary>
		DeviceError SetDieTemperature(MilliCelsius value);
		/// <summary>Reads FullCap register (35h), learned full capacity.</summary>
		[[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetLearnedFullCapacity() const;
		/// <summary>Writes FullCap register (35h), learned full capacity.</summary>
		DeviceError SetLearnedFullCapacity(MicroAmpereHours value);
		/// <summary>Reads Timer register (3Eh), low timer word. Internal register.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetTimerLowWord() const;
		/// <summary>Writes Timer register (3Eh), low timer word. Internal register.</summary>
		DeviceError SetTimerLowWord(uint16_t value);
		/// <summary>Reads ShdnTimer register (3Fh), shutdown timer. Internal register.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetShutdownTimerRaw() const;
		/// <summary>Writes ShdnTimer register (3Fh), shutdown timer. Internal register.</summary>
		DeviceError SetShutdownTimerRaw(uint16_t value);
		/// <summary>Reads QRTable30 register (42h). Internal model register.</summary>
		[[nodiscard]] std::expected<ModelQrParameter, DeviceError> GetModelQrTable30() const;
		/// <summary>Writes QRTable30 register (42h). Internal model register.</summary>
		DeviceError SetModelQrTable30(ModelQrParameter value);
		/// <summary>Reads RGain register (43h). Internal calibration register.</summary>
		[[nodiscard]] std::expected<ResistanceGain, DeviceError> GetResistanceGain() const;
		/// <summary>Writes RGain register (43h). Internal calibration register.</summary>
		DeviceError SetResistanceGain(ResistanceGain value);
		/// <summary>Reads dQAcc register (45h). Internal learning accumulator.</summary>
		[[nodiscard]] std::expected<CapacityAccumulator, DeviceError> GetCapacityAccumulator() const;
		/// <summary>Writes dQAcc register (45h). Internal learning accumulator.</summary>
		DeviceError SetCapacityAccumulator(CapacityAccumulator value);
		/// <summary>Reads dPAcc register (46h). Internal learning accumulator.</summary>
		[[nodiscard]] std::expected<PowerAccumulator, DeviceError> GetPowerAccumulator() const;
		/// <summary>Writes dPAcc register (46h). Internal learning accumulator.</summary>
		DeviceError SetPowerAccumulator(PowerAccumulator value);
		/// <summary>Reads ConvgCfg register (49h). Internal convergence register.</summary>
		[[nodiscard]] std::expected<ConvergenceConfiguration, DeviceError> GetConvergenceConfiguration() const;
		/// <summary>Writes ConvgCfg register (49h). Internal convergence register.</summary>
		DeviceError SetConvergenceConfiguration(ConvergenceConfiguration value);
		/// <summary>Reads VFRemCap register (4Ah), voltage-filtered remaining capacity.</summary>
		[[nodiscard]] std::expected<MicroAmpereHours, DeviceError> GetVoltageFilteredRemainingCapacity() const;
		/// <summary>Writes VFRemCap register (4Ah), voltage-filtered remaining capacity.</summary>
		DeviceError SetVoltageFilteredRemainingCapacity(MicroAmpereHours value);
		/// <summary>Reads QH register (4Dh). Internal coulomb-counter accumulator register.</summary>
		[[nodiscard]] std::expected<ChargeAccumulator, DeviceError> GetChargeAccumulator() const;
		/// <summary>Writes QH register (4Dh). Internal coulomb-counter accumulator register.</summary>
		DeviceError SetChargeAccumulator(ChargeAccumulator value);
		/// <summary>Reads Status2 register (B0h). Internal/extended status register.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetStatus2Raw() const;
		/// <summary>Writes Status2 register (B0h). Internal/extended status register.</summary>
		DeviceError SetStatus2Raw(uint16_t value);
		/// <summary>Reads Power register (B1h), instantaneous power in raw register format.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetInstantPowerRaw() const;
		/// <summary>Writes Power register (B1h), instantaneous power register value.</summary>
		DeviceError SetInstantPowerRaw(uint16_t value);
		/// <summary>Reads ID register (B2h), device ID or user memory alias.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetDeviceIdOrUserMem2() const;
		/// <summary>Writes UserMem2 alias at B2h (overlaps ID).</summary>
		DeviceError SetUserMem2(uint16_t value);
		/// <summary>Reads AvgPower register (B3h), averaged power in raw register format.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetAveragePowerRaw() const;
		/// <summary>Writes AvgPower register (B3h), averaged power register value.</summary>
		DeviceError SetAveragePowerRaw(uint16_t value);
		/// <summary>Reads TTFCfg register (B5h). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<TimeToFullConfiguration, DeviceError> GetTimeToFullConfiguration() const;
		/// <summary>Writes TTFCfg register (B5h). Internal algorithm register.</summary>
		DeviceError SetTimeToFullConfiguration(TimeToFullConfiguration value);
		/// <summary>Reads CVMixCap register (B6h). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<CvMixedCapacityParameter, DeviceError> GetCvMixedCapacity() const;
		/// <summary>Writes CVMixCap register (B6h). Internal algorithm register.</summary>
		DeviceError SetCvMixedCapacity(CvMixedCapacityParameter value);
		/// <summary>Reads CVHalfTime register (B7h). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<CvHalfTimeParameter, DeviceError> GetCvHalfTime() const;
		/// <summary>Writes CVHalfTime register (B7h). Internal algorithm register.</summary>
		DeviceError SetCvHalfTime(CvHalfTimeParameter value);
		/// <summary>Reads CGTempCo register (B8h). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<ChargeGainTemperatureCoefficient, DeviceError> GetChargeGainTemperatureCoefficient() const;
		/// <summary>Writes CGTempCo register (B8h). Internal algorithm register.</summary>
		DeviceError SetChargeGainTemperatureCoefficient(ChargeGainTemperatureCoefficient value);
		/// <summary>Reads Curve register (B9h). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<PowerCurveConfiguration, DeviceError> GetPowerCurveConfiguration() const;
		/// <summary>Writes Curve register (B9h). Internal algorithm register.</summary>
		DeviceError SetPowerCurveConfiguration(PowerCurveConfiguration value);
		/// <summary>Reads Config2 register (BBh), extended configuration flags.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetExtendedConfigurationRaw() const;
		/// <summary>Writes Config2 register (BBh), extended configuration flags.</summary>
		DeviceError SetExtendedConfigurationRaw(uint16_t value);
		/// <summary>Reads VRipple register (BCh). Internal measurement register.</summary>
		[[nodiscard]] std::expected<VoltageRippleMeasurement, DeviceError> GetVoltageRippleRaw() const;
		/// <summary>Writes VRipple register (BCh). Internal measurement register.</summary>
		DeviceError SetVoltageRippleRaw(VoltageRippleMeasurement value);
		/// <summary>Reads RippleCfg register (BDh). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<RippleConfiguration, DeviceError> GetRippleConfiguration() const;
		/// <summary>Writes RippleCfg register (BDh). Internal algorithm register.</summary>
		DeviceError SetRippleConfiguration(RippleConfiguration value);
		/// <summary>Reads TimerH register (BEh), high timer word. Internal register.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetTimerHighWord() const;
		/// <summary>Writes TimerH register (BEh), high timer word. Internal register.</summary>
		DeviceError SetTimerHighWord(uint16_t value);
		/// <summary>Reads RSense/UserMem3 register (D0h) raw value.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetSenseResistorOrUserMem3Raw() const;
		/// <summary>Writes RSense/UserMem3 register (D0h) raw value.</summary>
		DeviceError SetSenseResistorOrUserMem3Raw(uint16_t value);
		/// <summary>Reads ScOcvLim register (D1h). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<SocOcvLimitConfiguration, DeviceError> GetSocOcvLimitConfiguration() const;
		/// <summary>Writes ScOcvLim register (D1h). Internal algorithm register.</summary>
		DeviceError SetSocOcvLimitConfiguration(SocOcvLimitConfiguration value);
		/// <summary>Reads VGain register (D2h). Internal calibration register.</summary>
		[[nodiscard]] std::expected<VoltageGainCalibration, DeviceError> GetVoltageGain() const;
		/// <summary>Writes VGain register (D2h). Internal calibration register.</summary>
		DeviceError SetVoltageGain(VoltageGainCalibration value);
		/// <summary>Reads SOCHold register (D3h). Internal algorithm register.</summary>
		[[nodiscard]] std::expected<SocHoldConfiguration, DeviceError> GetSocHoldConfiguration() const;
		/// <summary>Writes SOCHold register (D3h). Internal algorithm register.</summary>
		DeviceError SetSocHoldConfiguration(SocHoldConfiguration value);
		/// <summary>Reads MaxPeakPower register (D4h), max peak power estimate raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetMaximumPeakPowerRaw() const;
		/// <summary>Writes MaxPeakPower register (D4h), max peak power estimate raw.</summary>
		DeviceError SetMaximumPeakPowerRaw(uint16_t value);
		/// <summary>Reads SusPeakPower register (D5h), sustained peak power estimate raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetSustainedPeakPowerRaw() const;
		/// <summary>Writes SusPeakPower register (D5h), sustained peak power estimate raw.</summary>
		DeviceError SetSustainedPeakPowerRaw(uint16_t value);
		/// <summary>Reads PackResistance register (D6h), pack resistance estimate raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetPackResistanceRaw() const;
		/// <summary>Writes PackResistance register (D6h), pack resistance estimate raw.</summary>
		DeviceError SetPackResistanceRaw(uint16_t value);
		/// <summary>Reads SysResistance register (D7h), system resistance estimate raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetSystemResistanceRaw() const;
		/// <summary>Writes SysResistance register (D7h), system resistance estimate raw.</summary>
		DeviceError SetSystemResistanceRaw(uint16_t value);
		/// <summary>Reads MinSysVoltage register (D8h), minimum system voltage raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetMinimumSystemVoltageRaw() const;
		/// <summary>Writes MinSysVoltage register (D8h), minimum system voltage raw.</summary>
		DeviceError SetMinimumSystemVoltageRaw(uint16_t value);
		/// <summary>Reads MPPCurrent register (D9h), max-peak-power current raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetMaximumPeakPowerCurrentRaw() const;
		/// <summary>Writes MPPCurrent register (D9h), max-peak-power current raw.</summary>
		DeviceError SetMaximumPeakPowerCurrentRaw(uint16_t value);
		/// <summary>Reads SPPCurrent register (DAh), sustained-peak-power current raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetSustainedPeakPowerCurrentRaw() const;
		/// <summary>Writes SPPCurrent register (DAh), sustained-peak-power current raw.</summary>
		DeviceError SetSustainedPeakPowerCurrentRaw(uint16_t value);
		/// <summary>Reads AtQResidual register (DCh), AtRate residual capacity prediction raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetAtRateResidualCapacityRaw() const;
		/// <summary>Writes AtQResidual register (DCh), AtRate residual capacity prediction raw.</summary>
		DeviceError SetAtRateResidualCapacityRaw(uint16_t value);
		/// <summary>Reads AtTTE register (DDh), AtRate time-to-empty prediction raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetAtRateTimeToEmptyRaw() const;
		/// <summary>Writes AtTTE register (DDh), AtRate time-to-empty prediction raw.</summary>
		DeviceError SetAtRateTimeToEmptyRaw(uint16_t value);
		/// <summary>Reads AtAvSOC register (DEh), AtRate average SOC prediction raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetAtRateAverageSocRaw() const;
		/// <summary>Writes AtAvSOC register (DEh), AtRate average SOC prediction raw.</summary>
		DeviceError SetAtRateAverageSocRaw(uint16_t value);
		/// <summary>Reads AtAvCap register (DFh), AtRate average capacity prediction raw.</summary>
		[[nodiscard]] std::expected<uint16_t, DeviceError> GetAtRateAverageCapacityRaw() const;
		/// <summary>Writes AtAvCap register (DFh), AtRate average capacity prediction raw.</summary>
		DeviceError SetAtRateAverageCapacityRaw(uint16_t value);

		/// <summary>
		/// Reads a group of learning/calibration registers used by ModelGauge learning.
		/// Internal registers.
		/// </summary>
		[[nodiscard]] std::expected<AlgorithmLearningParameters, DeviceError> GetAlgorithmLearningParameters() const;
		/// <summary>
		/// Writes a group of learning/calibration registers used by ModelGauge learning.
		/// Internal registers.
		/// </summary>
		DeviceError SetAlgorithmLearningParameters(const AlgorithmLearningParameters& value);

	private:
		I2C::Api::IDriver& m_Driver;

		DeviceError ReadRegisterRaw(RegOffset reg, uint8_t outData[2]) const;

		DeviceError WriteRegisterRaw(RegOffset reg, const uint8_t data[2]);

		template<typename T>
		[[nodiscard]] std::expected<T, DeviceError> ReadRegister(RegOffset reg) const
		{
			uint8_t data[2];
			if (auto error = ReadRegisterRaw(reg, data); error != DeviceError::None)
			{
				return std::unexpected(error);
			}

			return RegUtils::Read<T, std::endian::little>(data, 0, 16);
		}

		template<typename T>
		DeviceError WriteRegister(RegOffset reg, T value)
		{
			uint8_t data[2] = {};
			RegUtils::Write<T, std::endian::little>(value, data, 0, 16);
			return WriteRegisterRaw(reg, data);
		}

		template<typename T>
		[[nodiscard]] std::expected<T, DeviceError> ReadField(RegOffset reg, size_t startBit, size_t bitCount) const
		{
			uint8_t data[2];
			if (auto error = ReadRegisterRaw(reg, data); error != DeviceError::None)
			{
				return std::unexpected(error);
			}

			return RegUtils::Read<T, std::endian::little>(data, startBit, bitCount);
		}

		template<typename T>
		DeviceError WriteField(RegOffset reg, size_t startBit, size_t bitCount, T value)
		{
			uint8_t data[2];
			if (auto error = ReadRegisterRaw(reg, data); error != DeviceError::None)
			{
				return error;
			}

			RegUtils::Write<T, std::endian::little>(value, data, startBit, bitCount);
			return WriteRegisterRaw(reg, data);
		}
	};
};
