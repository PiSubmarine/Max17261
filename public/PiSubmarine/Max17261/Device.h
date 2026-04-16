#pragma once

#include "PiSubmarine/RegUtils.h"
#include "PiSubmarine/Max17261/MicroAmpereHours.h"
#include "PiSubmarine/Max17261/MicroAmperes.h"
#include "PiSubmarine/Max17261/MicroVolts.h"
#include "PiSubmarine/Max17261/MilliCelcius.h"
#include "PiSubmarine/NormalizedIntFraction.h"
#include "PiSubmarine/Error/Api/Result.h"
#include "PiSubmarine/I2C/Api/IDriver.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <chrono>
#include <array>

namespace PiSubmarine::Max17261
{
	using NormalizedIntFactor = PiSubmarine::NormalizedIntFraction<16>;

	using WaitFunc = std::function<void(std::chrono::milliseconds)>;

	template<typename T>
	using Result = PiSubmarine::Error::Api::Result<T>;

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
		/// Constructs a MAX172611 device wrapper using the provided blocking I2C driver.
		/// </summary>
		Device(I2C::Api::IDriver& driver);

		/// <summary>
		/// Initializes MAX17261 in blocking mode. Must be called on every power cycle.
		/// </summary>
		/// <returns>An empty result on success, or an error if initialization fails.</returns>
		Result<void> Init(const WaitFunc& waitFunc, MicroAmpereHours designCapacity, MicroAmperes terminationCurrent, MicroVolts emptyVoltage, bool forceReset = false);

		/// <summary>
		/// Reads Status register (00h) alert/status flags.
		/// </summary>
		[[nodiscard]] Result<Status> GetAlertStatus() const;
		/// <summary>
		/// Writes Status register (00h), typically to clear sticky alert bits.
		/// </summary>
		Result<void> SetAlertStatus(Status value);
		/// <summary>
		/// Reads FuelGaugeStatus register (3Dh) fuel-gauge algorithm status.
		/// </summary>
		[[nodiscard]] Result<FuelGaugeStatus> GetFuelGaugeStatus() const;
		/// <summary>
		/// Reads HibCfg.HibScalar (BAh[2:0]) hibernate task period scale.
		/// </summary>
		[[nodiscard]] Result<uint8_t> GetHibernateTaskPeriodScale() const;

		/// <summary>
		/// Sets HibCfg.HibScalar (BAh[2:0]).
		/// Task period is approximately 351ms * 2^HibScalar.
		/// </summary>
		Result<void> SetHibernateTaskPeriodScale(uint8_t value);

		/// <summary>
		/// Reads HibCfg.HibExitTime (BAh[4:3]) hibernate exit delay field.
		/// </summary>
		[[nodiscard]] Result<uint8_t> GetHibernateExitDelay() const;

		/// <summary>
		/// Sets HibCfg.HibExitTime (BAh[4:3]).
		/// Exit delay is approximately (HibExitTime + 1) * 702ms * 2^HibScalar.
		/// </summary>
		Result<void> SetHibernateExitDelay(uint8_t value);

		/// <summary>
		/// Reads HibCfg.HibThreshold (BAh[11:8]) current threshold for hibernate logic.
		/// </summary>
		[[nodiscard]] Result<uint8_t> GetHibernateCurrentThreshold() const;

		/// <summary>
		/// Sets HibCfg.HibThreshold (BAh[11:8]) current threshold for hibernate logic.
		/// </summary>
		Result<void> SetHibernateCurrentThreshold(uint8_t value);

		/// <summary>
		/// Reads HibCfg.HibEnterTime (BAh[14:12]) hibernate entry delay.
		/// </summary>
		[[nodiscard]] Result<uint8_t> GetHibernateEnterDelay() const;

		/// <summary>
		/// Sets HibCfg.HibEnterTime (BAh[14:12]) hibernate entry delay.
		/// </summary>
		Result<void> SetHibernateEnterDelay(uint8_t value);

		/// <summary>
		/// Reads HibCfg.EnHib (BAh[15]) hibernate enable state.
		/// </summary>
		[[nodiscard]] Result<bool> IsHibernateModeEnabled() const;

		/// <summary>
		/// Sets HibCfg.EnHib (BAh[15]) hibernate enable state.
		/// </summary>
		Result<void> SetHibernateModeEnabled(bool value);

		/// <summary>
		/// Writes Command register (60h) with control command (Clear/Reset/SoftWakeup).
		/// </summary>
		Result<void> ExecuteCommand(Command command);

		/// <summary>
		/// Reads Command register (60h).
		/// </summary>
		[[nodiscard]] Result<Command> GetCommandRegisterValue() const;

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Writes DesignCap register (18h), design capacity.
		Result<void> SetDesignCapacity(MicroAmpereHours valueMah);

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Reads DesignCap register (18h), design capacity.
		[[nodiscard]] Result<MicroAmpereHours> GetDesignCapacity() const;

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Writes IChgTerm register (1Eh), charge termination current.
		Result<void> SetChargeTerminationCurrent(MicroAmperes valueMa);

		/// <summary>
		/// Assumes 0.010 Ohm sense resistor.
		/// </summary>
		/// Reads IChgTerm register (1Eh), charge termination current.
		[[nodiscard]] Result<MicroAmperes> GetChargeTerminationCurrent() const;

		/// <summary>
		/// Writes VEmpty.VEmpty (3Ah[15:7]) battery empty voltage threshold.
		/// </summary>
		Result<void> SetBatteryEmptyVoltage(MicroVolts valueUv);

		/// <summary>
		/// Reads VEmpty.VEmpty (3Ah[15:7]) battery empty voltage threshold.
		/// </summary>
		[[nodiscard]] Result<MicroVolts> GetBatteryEmptyVoltage() const;

		/// <summary>
		/// Writes VEmpty.VRecovery (3Ah[6:0]) battery recovery voltage threshold.
		/// </summary>
		Result<void> SetBatteryRecoveryVoltage(MicroVolts valueUv);

		/// <summary>
		/// Reads VEmpty.VRecovery (3Ah[6:0]) battery recovery voltage threshold.
		/// </summary>
		[[nodiscard]] Result<MicroVolts> GetBatteryRecoveryVoltage() const;

		/// <summary>
		/// Reads ModelCfg.ModelID (DBh[7:4]) battery chemistry model selection.
		/// </summary>
		[[nodiscard]] Result<ModelId> GetBatteryChemistryModel() const;

		/// <summary>
		/// Writes ModelCfg.ModelID (DBh[7:4]) battery chemistry model selection.
		/// </summary>
		Result<void> SetBatteryChemistryModel(ModelId value);

		/// <summary>
		/// Reads ModelCfg.VChg (DBh[10]) high-voltage charge profile selection.
		/// </summary>
		[[nodiscard]] Result<bool> IsHighVoltageChargeProfileEnabled() const;

		/// <summary>
		/// Writes ModelCfg.VChg (DBh[10]) high-voltage charge profile selection.
		/// </summary>
		Result<void> SetHighVoltageChargeProfileEnabled(bool value);

		/// <summary>
		/// Reads ModelCfg.Refresh (DBh[15]) model refresh in-progress flag.
		/// </summary>
		[[nodiscard]] Result<bool> IsModelRefreshInProgress() const;

		/// <summary>
		/// Writes ModelCfg.Refresh (DBh[15]) to request model refresh.
		/// </summary>
		Result<void> SetModelRefreshRequested(bool value);

		/// <summary>
		/// Reads RepCap register (05h), reported remaining capacity estimate.
		/// </summary>
		[[nodiscard]] Result<MicroAmpereHours> GetRemainingCapacityEstimate() const;

		/// <summary>
		/// Reads FullCapRep register (10h), reported full capacity estimate.
		/// </summary>
		[[nodiscard]] Result<MicroAmpereHours> GetReportedFullCapacityEstimate() const;

		/// <summary>
		/// Writes FullCapRep register (10h), reported full capacity estimate.
		/// </summary>
		Result<void> SetReportedFullCapacityEstimate(MicroAmpereHours valueuAh);

		/// <summary>
		/// Reads FullCapNom register (23h), nominal full capacity estimate.
		/// </summary>
		[[nodiscard]] Result<MicroAmpereHours> GetNominalFullCapacityEstimate() const;

		/// <summary>
		/// Writes FullCapNom register (23h), nominal full capacity estimate.
		/// </summary>
		Result<void> SetNominalFullCapacityEstimate(MicroAmpereHours cap);

		/// <summary>
		/// Reads RepSOC register (06h) as normalized state-of-charge factor.
		/// Datasheet LSB is 1/256% (0x6400 = 100%).
		/// </summary>
		[[nodiscard]] Result<NormalizedIntFactor> GetRemainingStateOfCharge() const;

		/// <summary>
		/// Reads Current register (0Ah), instantaneous pack current.
		/// </summary>
		[[nodiscard]] Result<MicroAmperes> GetInstantCurrent() const;

		/// <summary>
		/// Reads TTE register (11h), time-to-empty in raw register units.
		/// </summary>
		[[nodiscard]] Result<uint16_t> GetTimeToEmptyRaw() const;

		/// <summary>
		/// Reads TTF register (20h), time-to-full in raw register units.
		/// </summary>
		[[nodiscard]] Result<uint16_t> GetTimeToFullRaw() const;

		/// <summary>
		/// Reads Cycles register (17h), cycle counter.
		/// </summary>
		[[nodiscard]] Result<uint16_t> GetCycleCount() const;

		/// <summary>
		/// Writes Cycles register (17h), cycle counter.
		/// </summary>
		Result<void> SetCycleCount(uint16_t value);

		/// <summary>
		/// Reads RComp0 register (38h), temperature compensation baseline value.
		/// </summary>
		[[nodiscard]] Result<uint16_t> GetTemperatureCompensationBaseline() const;

		/// <summary>
		/// Writes RComp0 register (38h), temperature compensation baseline value.
		/// </summary>
		Result<void> SetTemperatureCompensationBaseline(uint16_t value);

		/// <summary>
		/// Reads TempCo register (39h), temperature compensation coefficient.
		/// </summary>
		[[nodiscard]] Result<uint16_t> GetTemperatureCompensationCoefficient() const;

		/// <summary>
		/// Writes TempCo register (39h), temperature compensation coefficient.
		/// </summary>
		Result<void> SetTemperatureCompensationCoefficient(uint16_t value);

		/// <summary>
		/// Reads VCell register (09h), instantaneous cell/pack voltage.
		/// </summary>
		[[nodiscard]] Result<MicroVolts> GetInstantVoltage() const;

		/// <summary>
		/// Reads Config register (1Dh), operation and alert-configuration flags.
		/// </summary>
		[[nodiscard]] Result<ConfigFlags> GetDeviceConfiguration() const;

		/// <summary>
		/// Writes Config register (1Dh), operation and alert-configuration flags.
		/// </summary>
		Result<void> SetDeviceConfiguration(ConfigFlags value);

		/// <summary>
		/// Reads VAlrtTh register (01h) voltage alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] Result<AlertThresholdRaw8> GetVoltageAlertThresholdRaw() const;
		/// <summary>
		/// Writes VAlrtTh register (01h) voltage alert thresholds (raw encoded bytes).
		/// </summary>
		Result<void> SetVoltageAlertThresholdRaw(const AlertThresholdRaw8& value);
		/// <summary>
		/// Reads TAlrtTh register (02h) temperature alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] Result<AlertThresholdRaw8> GetTemperatureAlertThresholdRaw() const;
		/// <summary>
		/// Writes TAlrtTh register (02h) temperature alert thresholds (raw encoded bytes).
		/// </summary>
		Result<void> SetTemperatureAlertThresholdRaw(const AlertThresholdRaw8& value);
		/// <summary>
		/// Reads SAlrtTh register (03h) SOC alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] Result<AlertThresholdRaw8> GetSocAlertThresholdRaw() const;
		/// <summary>
		/// Writes SAlrtTh register (03h) SOC alert thresholds (raw encoded bytes).
		/// </summary>
		Result<void> SetSocAlertThresholdRaw(const AlertThresholdRaw8& value);
		/// <summary>
		/// Reads IAlrtTh register (B4h) current alert thresholds (raw encoded bytes).
		/// </summary>
		[[nodiscard]] Result<AlertThresholdRaw8> GetCurrentAlertThresholdRaw() const;
		/// <summary>
		/// Writes IAlrtTh register (B4h) current alert thresholds (raw encoded bytes).
		/// </summary>
		Result<void> SetCurrentAlertThresholdRaw(const AlertThresholdRaw8& value);

		/// <summary>Reads AtRate register (04h), theoretical load current input.</summary>
		[[nodiscard]] Result<MicroAmperes> GetTheoreticalLoadCurrent() const;
		/// <summary>Writes AtRate register (04h), theoretical load current input.</summary>
		Result<void> SetTheoreticalLoadCurrent(MicroAmperes value);

		/// <summary>Reads Age register (07h) as normalized age/capacity ratio factor (Age%).</summary>
		[[nodiscard]] Result<NormalizedIntFactor> GetAgeEstimate() const;
		/// <summary>Writes Age register (07h) as normalized age/capacity ratio factor (Age%).</summary>
		Result<void> SetAgeEstimate(NormalizedIntFactor value);
		/// <summary>Reads Temp register (08h) measured battery temperature.</summary>
		[[nodiscard]] Result<MilliCelsius> GetBatteryTemperature() const;
		/// <summary>Writes Temp register (08h) measured battery temperature register value.</summary>
		Result<void> SetBatteryTemperature(MilliCelsius value);
		/// <summary>Reads AvgCurrent register (0Bh), averaged battery current.</summary>
		[[nodiscard]] Result<MicroAmperes> GetAverageCurrent() const;
		/// <summary>Writes AvgCurrent register (0Bh), averaged battery current register value.</summary>
		Result<void> SetAverageCurrent(MicroAmperes value);
		/// <summary>Reads QResidual register (0Ch), internal residual capacity term. Internal register.</summary>
		[[nodiscard]] Result<CapacityAccumulator> GetResidualCapacityTerm() const;
		/// <summary>Writes QResidual register (0Ch). Internal register.</summary>
		Result<void> SetResidualCapacityTerm(CapacityAccumulator value);
		/// <summary>Reads MixSOC register (0Dh), mixed SOC estimate as normalized factor (1/256% LSB).</summary>
		[[nodiscard]] Result<NormalizedIntFactor> GetMixedSoc() const;
		/// <summary>Writes MixSOC register (0Dh), mixed SOC estimate as normalized factor (1/256% LSB).</summary>
		Result<void> SetMixedSoc(NormalizedIntFactor value);
		/// <summary>Reads AvSOC register (0Eh), filtered SOC estimate as normalized factor (1/256% LSB).</summary>
		[[nodiscard]] Result<NormalizedIntFactor> GetFilteredSoc() const;
		/// <summary>Writes AvSOC register (0Eh), filtered SOC estimate as normalized factor (1/256% LSB).</summary>
		Result<void> SetFilteredSoc(NormalizedIntFactor value);
		/// <summary>Reads MixCap register (0Fh), mixed capacity estimate.</summary>
		[[nodiscard]] Result<MicroAmpereHours> GetMixedCapacityEstimate() const;
		/// <summary>Writes MixCap register (0Fh), mixed capacity estimate.</summary>
		Result<void> SetMixedCapacityEstimate(MicroAmpereHours value);
		/// <summary>Reads QRTable00 register (12h). Internal model register.</summary>
		[[nodiscard]] Result<ModelQrParameter> GetModelQrTable00() const;
		/// <summary>Writes QRTable00 register (12h). Internal model register.</summary>
		Result<void> SetModelQrTable00(ModelQrParameter value);
		/// <summary>Reads FullSocThr register (13h) full SOC threshold as normalized factor (1/256% LSB).</summary>
		[[nodiscard]] Result<NormalizedIntFactor> GetFullSocThreshold() const;
		/// <summary>Writes FullSocThr register (13h) full SOC threshold as normalized factor (1/256% LSB).</summary>
		Result<void> SetFullSocThreshold(NormalizedIntFactor value);
		/// <summary>Reads RCell register (14h), internal resistance estimate in raw format.</summary>
		[[nodiscard]] Result<uint16_t> GetCellResistanceRaw() const;
		/// <summary>Writes RCell register (14h), internal resistance estimate in raw format.</summary>
		Result<void> SetCellResistanceRaw(uint16_t value);
		/// <summary>Reads AvgTA register (16h), averaged temperature.</summary>
		[[nodiscard]] Result<MilliCelsius> GetAverageTemperature() const;
		/// <summary>Writes AvgTA register (16h), averaged temperature register value.</summary>
		Result<void> SetAverageTemperature(MilliCelsius value);
		/// <summary>Reads AvgVCell register (19h), averaged voltage.</summary>
		[[nodiscard]] Result<MicroVolts> GetAverageVoltage() const;
		/// <summary>Writes AvgVCell register (19h), averaged voltage register value.</summary>
		Result<void> SetAverageVoltage(MicroVolts value);
		/// <summary>Reads MaxMinTemp register (1Ah), max/min temperature bytes in one transaction.</summary>
		[[nodiscard]] Result<MinMaxRaw8> GetTemperatureMinMaxRaw() const;
		/// <summary>Writes MaxMinTemp register (1Ah), max/min temperature bytes in one transaction.</summary>
		Result<void> SetTemperatureMinMaxRaw(const MinMaxRaw8& value);
		/// <summary>Reads MaxMinVolt register (1Bh), max/min voltage bytes in one transaction.</summary>
		[[nodiscard]] Result<MinMaxRaw8> GetVoltageMinMaxRaw() const;
		/// <summary>Writes MaxMinVolt register (1Bh), max/min voltage bytes in one transaction.</summary>
		Result<void> SetVoltageMinMaxRaw(const MinMaxRaw8& value);
		/// <summary>Reads MaxMinCurr register (1Ch), max/min current bytes in one transaction.</summary>
		[[nodiscard]] Result<MinMaxRaw8> GetCurrentMinMaxRaw() const;
		/// <summary>Writes MaxMinCurr register (1Ch), max/min current bytes in one transaction.</summary>
		Result<void> SetCurrentMinMaxRaw(const MinMaxRaw8& value);
		/// <summary>Reads AvCap register (1Fh), filtered available capacity estimate.</summary>
		[[nodiscard]] Result<MicroAmpereHours> GetFilteredAvailableCapacityEstimate() const;
		/// <summary>Writes AvCap register (1Fh), filtered available capacity estimate.</summary>
		Result<void> SetFilteredAvailableCapacityEstimate(MicroAmpereHours value);
		/// <summary>Reads DevName register (21h) silicon device identifier.</summary>
		[[nodiscard]] Result<uint16_t> GetDeviceNameCode() const;
		/// <summary>Reads QRTable10 register (22h). Internal model register.</summary>
		[[nodiscard]] Result<ModelQrParameter> GetModelQrTable10() const;
		/// <summary>Writes QRTable10 register (22h). Internal model register.</summary>
		Result<void> SetModelQrTable10(ModelQrParameter value);
		/// <summary>Reads AIN register (27h), auxiliary ADC input in raw format.</summary>
		[[nodiscard]] Result<uint16_t> GetAuxInputRaw() const;
		/// <summary>Writes AIN register (27h), auxiliary ADC input register value.</summary>
		Result<void> SetAuxInputRaw(uint16_t value);
		/// <summary>Reads LearnCfg register (28h). Internal learning register.</summary>
		[[nodiscard]] Result<LearnConfiguration> GetLearnConfiguration() const;
		/// <summary>Writes LearnCfg register (28h). Internal learning register.</summary>
		Result<void> SetLearnConfiguration(LearnConfiguration value);
		/// <summary>Reads FilterCfg register (29h). Internal filter register.</summary>
		[[nodiscard]] Result<FilterConfiguration> GetFilterConfiguration() const;
		/// <summary>Writes FilterCfg register (29h). Internal filter register.</summary>
		Result<void> SetFilterConfiguration(FilterConfiguration value);
		/// <summary>Reads RelaxCfg register (2Ah). Internal relax register.</summary>
		[[nodiscard]] Result<RelaxConfiguration> GetRelaxConfiguration() const;
		/// <summary>Writes RelaxCfg register (2Ah). Internal relax register.</summary>
		Result<void> SetRelaxConfiguration(RelaxConfiguration value);
		/// <summary>Reads MiscCfg register (2Bh). Internal algorithm register.</summary>
		[[nodiscard]] Result<MiscConfiguration> GetMiscConfiguration() const;
		/// <summary>Writes MiscCfg register (2Bh). Internal algorithm register.</summary>
		Result<void> SetMiscConfiguration(MiscConfiguration value);
		/// <summary>Reads TGain register (2Ch). Internal calibration register.</summary>
		[[nodiscard]] Result<TemperatureGain> GetTemperatureGain() const;
		/// <summary>Writes TGain register (2Ch). Internal calibration register.</summary>
		Result<void> SetTemperatureGain(TemperatureGain value);
		/// <summary>Reads TOff register (2Dh). Internal calibration register.</summary>
		[[nodiscard]] Result<TemperatureOffset> GetTemperatureOffset() const;
		/// <summary>Writes TOff register (2Dh). Internal calibration register.</summary>
		Result<void> SetTemperatureOffset(TemperatureOffset value);
		/// <summary>Reads CGain register (2Eh). Internal calibration register.</summary>
		[[nodiscard]] Result<CurrentGain> GetCurrentGain() const;
		/// <summary>Writes CGain register (2Eh). Internal calibration register.</summary>
		Result<void> SetCurrentGain(CurrentGain value);
		/// <summary>Reads COff register (2Fh). Internal calibration register.</summary>
		[[nodiscard]] Result<CurrentOffset> GetCurrentOffset() const;
		/// <summary>Writes COff register (2Fh). Internal calibration register.</summary>
		Result<void> SetCurrentOffset(CurrentOffset value);
		/// <summary>Reads QRTable20 register (32h). Internal model register.</summary>
		[[nodiscard]] Result<ModelQrParameter> GetModelQrTable20() const;
		/// <summary>Writes QRTable20 register (32h). Internal model register.</summary>
		Result<void> SetModelQrTable20(ModelQrParameter value);
		/// <summary>Reads DieTemp register (34h), internal IC die temperature.</summary>
		[[nodiscard]] Result<MilliCelsius> GetDieTemperature() const;
		/// <summary>Writes DieTemp register (34h), internal IC die temperature register value.</summary>
		Result<void> SetDieTemperature(MilliCelsius value);
		/// <summary>Reads FullCap register (35h), learned full capacity.</summary>
		[[nodiscard]] Result<MicroAmpereHours> GetLearnedFullCapacity() const;
		/// <summary>Writes FullCap register (35h), learned full capacity.</summary>
		Result<void> SetLearnedFullCapacity(MicroAmpereHours value);
		/// <summary>Reads Timer register (3Eh), low timer word. Internal register.</summary>
		[[nodiscard]] Result<uint16_t> GetTimerLowWord() const;
		/// <summary>Writes Timer register (3Eh), low timer word. Internal register.</summary>
		Result<void> SetTimerLowWord(uint16_t value);
		/// <summary>Reads ShdnTimer register (3Fh), shutdown timer. Internal register.</summary>
		[[nodiscard]] Result<uint16_t> GetShutdownTimerRaw() const;
		/// <summary>Writes ShdnTimer register (3Fh), shutdown timer. Internal register.</summary>
		Result<void> SetShutdownTimerRaw(uint16_t value);
		/// <summary>Reads QRTable30 register (42h). Internal model register.</summary>
		[[nodiscard]] Result<ModelQrParameter> GetModelQrTable30() const;
		/// <summary>Writes QRTable30 register (42h). Internal model register.</summary>
		Result<void> SetModelQrTable30(ModelQrParameter value);
		/// <summary>Reads RGain register (43h). Internal calibration register.</summary>
		[[nodiscard]] Result<ResistanceGain> GetResistanceGain() const;
		/// <summary>Writes RGain register (43h). Internal calibration register.</summary>
		Result<void> SetResistanceGain(ResistanceGain value);
		/// <summary>Reads dQAcc register (45h). Internal learning accumulator.</summary>
		[[nodiscard]] Result<CapacityAccumulator> GetCapacityAccumulator() const;
		/// <summary>Writes dQAcc register (45h). Internal learning accumulator.</summary>
		Result<void> SetCapacityAccumulator(CapacityAccumulator value);
		/// <summary>Reads dPAcc register (46h). Internal learning accumulator.</summary>
		[[nodiscard]] Result<PowerAccumulator> GetPowerAccumulator() const;
		/// <summary>Writes dPAcc register (46h). Internal learning accumulator.</summary>
		Result<void> SetPowerAccumulator(PowerAccumulator value);
		/// <summary>Reads ConvgCfg register (49h). Internal convergence register.</summary>
		[[nodiscard]] Result<ConvergenceConfiguration> GetConvergenceConfiguration() const;
		/// <summary>Writes ConvgCfg register (49h). Internal convergence register.</summary>
		Result<void> SetConvergenceConfiguration(ConvergenceConfiguration value);
		/// <summary>Reads VFRemCap register (4Ah), voltage-filtered remaining capacity.</summary>
		[[nodiscard]] Result<MicroAmpereHours> GetVoltageFilteredRemainingCapacity() const;
		/// <summary>Writes VFRemCap register (4Ah), voltage-filtered remaining capacity.</summary>
		Result<void> SetVoltageFilteredRemainingCapacity(MicroAmpereHours value);
		/// <summary>Reads QH register (4Dh). Internal coulomb-counter accumulator register.</summary>
		[[nodiscard]] Result<ChargeAccumulator> GetChargeAccumulator() const;
		/// <summary>Writes QH register (4Dh). Internal coulomb-counter accumulator register.</summary>
		Result<void> SetChargeAccumulator(ChargeAccumulator value);
		/// <summary>Reads Status2 register (B0h). Internal/extended status register.</summary>
		[[nodiscard]] Result<uint16_t> GetStatus2Raw() const;
		/// <summary>Writes Status2 register (B0h). Internal/extended status register.</summary>
		Result<void> SetStatus2Raw(uint16_t value);
		/// <summary>Reads Power register (B1h), instantaneous power in raw register format.</summary>
		[[nodiscard]] Result<uint16_t> GetInstantPowerRaw() const;
		/// <summary>Writes Power register (B1h), instantaneous power register value.</summary>
		Result<void> SetInstantPowerRaw(uint16_t value);
		/// <summary>Reads ID register (B2h), device ID or user memory alias.</summary>
		[[nodiscard]] Result<uint16_t> GetDeviceIdOrUserMem2() const;
		/// <summary>Writes UserMem2 alias at B2h (overlaps ID).</summary>
		Result<void> SetUserMem2(uint16_t value);
		/// <summary>Reads AvgPower register (B3h), averaged power in raw register format.</summary>
		[[nodiscard]] Result<uint16_t> GetAveragePowerRaw() const;
		/// <summary>Writes AvgPower register (B3h), averaged power register value.</summary>
		Result<void> SetAveragePowerRaw(uint16_t value);
		/// <summary>Reads TTFCfg register (B5h). Internal algorithm register.</summary>
		[[nodiscard]] Result<TimeToFullConfiguration> GetTimeToFullConfiguration() const;
		/// <summary>Writes TTFCfg register (B5h). Internal algorithm register.</summary>
		Result<void> SetTimeToFullConfiguration(TimeToFullConfiguration value);
		/// <summary>Reads CVMixCap register (B6h). Internal algorithm register.</summary>
		[[nodiscard]] Result<CvMixedCapacityParameter> GetCvMixedCapacity() const;
		/// <summary>Writes CVMixCap register (B6h). Internal algorithm register.</summary>
		Result<void> SetCvMixedCapacity(CvMixedCapacityParameter value);
		/// <summary>Reads CVHalfTime register (B7h). Internal algorithm register.</summary>
		[[nodiscard]] Result<CvHalfTimeParameter> GetCvHalfTime() const;
		/// <summary>Writes CVHalfTime register (B7h). Internal algorithm register.</summary>
		Result<void> SetCvHalfTime(CvHalfTimeParameter value);
		/// <summary>Reads CGTempCo register (B8h). Internal algorithm register.</summary>
		[[nodiscard]] Result<ChargeGainTemperatureCoefficient> GetChargeGainTemperatureCoefficient() const;
		/// <summary>Writes CGTempCo register (B8h). Internal algorithm register.</summary>
		Result<void> SetChargeGainTemperatureCoefficient(ChargeGainTemperatureCoefficient value);
		/// <summary>Reads Curve register (B9h). Internal algorithm register.</summary>
		[[nodiscard]] Result<PowerCurveConfiguration> GetPowerCurveConfiguration() const;
		/// <summary>Writes Curve register (B9h). Internal algorithm register.</summary>
		Result<void> SetPowerCurveConfiguration(PowerCurveConfiguration value);
		/// <summary>Reads Config2 register (BBh), extended configuration flags.</summary>
		[[nodiscard]] Result<uint16_t> GetExtendedConfigurationRaw() const;
		/// <summary>Writes Config2 register (BBh), extended configuration flags.</summary>
		Result<void> SetExtendedConfigurationRaw(uint16_t value);
		/// <summary>Reads VRipple register (BCh). Internal measurement register.</summary>
		[[nodiscard]] Result<VoltageRippleMeasurement> GetVoltageRippleRaw() const;
		/// <summary>Writes VRipple register (BCh). Internal measurement register.</summary>
		Result<void> SetVoltageRippleRaw(VoltageRippleMeasurement value);
		/// <summary>Reads RippleCfg register (BDh). Internal algorithm register.</summary>
		[[nodiscard]] Result<RippleConfiguration> GetRippleConfiguration() const;
		/// <summary>Writes RippleCfg register (BDh). Internal algorithm register.</summary>
		Result<void> SetRippleConfiguration(RippleConfiguration value);
		/// <summary>Reads TimerH register (BEh), high timer word. Internal register.</summary>
		[[nodiscard]] Result<uint16_t> GetTimerHighWord() const;
		/// <summary>Writes TimerH register (BEh), high timer word. Internal register.</summary>
		Result<void> SetTimerHighWord(uint16_t value);
		/// <summary>Reads RSense/UserMem3 register (D0h) raw value.</summary>
		[[nodiscard]] Result<uint16_t> GetSenseResistorOrUserMem3Raw() const;
		/// <summary>Writes RSense/UserMem3 register (D0h) raw value.</summary>
		Result<void> SetSenseResistorOrUserMem3Raw(uint16_t value);
		/// <summary>Reads ScOcvLim register (D1h). Internal algorithm register.</summary>
		[[nodiscard]] Result<SocOcvLimitConfiguration> GetSocOcvLimitConfiguration() const;
		/// <summary>Writes ScOcvLim register (D1h). Internal algorithm register.</summary>
		Result<void> SetSocOcvLimitConfiguration(SocOcvLimitConfiguration value);
		/// <summary>Reads VGain register (D2h). Internal calibration register.</summary>
		[[nodiscard]] Result<VoltageGainCalibration> GetVoltageGain() const;
		/// <summary>Writes VGain register (D2h). Internal calibration register.</summary>
		Result<void> SetVoltageGain(VoltageGainCalibration value);
		/// <summary>Reads SOCHold register (D3h). Internal algorithm register.</summary>
		[[nodiscard]] Result<SocHoldConfiguration> GetSocHoldConfiguration() const;
		/// <summary>Writes SOCHold register (D3h). Internal algorithm register.</summary>
		Result<void> SetSocHoldConfiguration(SocHoldConfiguration value);
		/// <summary>Reads MaxPeakPower register (D4h), max peak power estimate raw.</summary>
		[[nodiscard]] Result<uint16_t> GetMaximumPeakPowerRaw() const;
		/// <summary>Writes MaxPeakPower register (D4h), max peak power estimate raw.</summary>
		Result<void> SetMaximumPeakPowerRaw(uint16_t value);
		/// <summary>Reads SusPeakPower register (D5h), sustained peak power estimate raw.</summary>
		[[nodiscard]] Result<uint16_t> GetSustainedPeakPowerRaw() const;
		/// <summary>Writes SusPeakPower register (D5h), sustained peak power estimate raw.</summary>
		Result<void> SetSustainedPeakPowerRaw(uint16_t value);
		/// <summary>Reads PackResistance register (D6h), pack resistance estimate raw.</summary>
		[[nodiscard]] Result<uint16_t> GetPackResistanceRaw() const;
		/// <summary>Writes PackResistance register (D6h), pack resistance estimate raw.</summary>
		Result<void> SetPackResistanceRaw(uint16_t value);
		/// <summary>Reads SysResistance register (D7h), system resistance estimate raw.</summary>
		[[nodiscard]] Result<uint16_t> GetSystemResistanceRaw() const;
		/// <summary>Writes SysResistance register (D7h), system resistance estimate raw.</summary>
		Result<void> SetSystemResistanceRaw(uint16_t value);
		/// <summary>Reads MinSysVoltage register (D8h), minimum system voltage raw.</summary>
		[[nodiscard]] Result<uint16_t> GetMinimumSystemVoltageRaw() const;
		/// <summary>Writes MinSysVoltage register (D8h), minimum system voltage raw.</summary>
		Result<void> SetMinimumSystemVoltageRaw(uint16_t value);
		/// <summary>Reads MPPCurrent register (D9h), max-peak-power current raw.</summary>
		[[nodiscard]] Result<uint16_t> GetMaximumPeakPowerCurrentRaw() const;
		/// <summary>Writes MPPCurrent register (D9h), max-peak-power current raw.</summary>
		Result<void> SetMaximumPeakPowerCurrentRaw(uint16_t value);
		/// <summary>Reads SPPCurrent register (DAh), sustained-peak-power current raw.</summary>
		[[nodiscard]] Result<uint16_t> GetSustainedPeakPowerCurrentRaw() const;
		/// <summary>Writes SPPCurrent register (DAh), sustained-peak-power current raw.</summary>
		Result<void> SetSustainedPeakPowerCurrentRaw(uint16_t value);
		/// <summary>Reads AtQResidual register (DCh), AtRate residual capacity prediction raw.</summary>
		[[nodiscard]] Result<uint16_t> GetAtRateResidualCapacityRaw() const;
		/// <summary>Writes AtQResidual register (DCh), AtRate residual capacity prediction raw.</summary>
		Result<void> SetAtRateResidualCapacityRaw(uint16_t value);
		/// <summary>Reads AtTTE register (DDh), AtRate time-to-empty prediction raw.</summary>
		[[nodiscard]] Result<uint16_t> GetAtRateTimeToEmptyRaw() const;
		/// <summary>Writes AtTTE register (DDh), AtRate time-to-empty prediction raw.</summary>
		Result<void> SetAtRateTimeToEmptyRaw(uint16_t value);
		/// <summary>Reads AtAvSOC register (DEh), AtRate average SOC prediction as normalized factor (1/256% LSB).</summary>
		[[nodiscard]] Result<NormalizedIntFactor> GetAtRateAverageSoc() const;
		/// <summary>Writes AtAvSOC register (DEh), AtRate average SOC prediction as normalized factor (1/256% LSB).</summary>
		Result<void> SetAtRateAverageSoc(NormalizedIntFactor value);
		/// <summary>Reads AtAvCap register (DFh), AtRate average capacity prediction raw.</summary>
		[[nodiscard]] Result<uint16_t> GetAtRateAverageCapacityRaw() const;
		/// <summary>Writes AtAvCap register (DFh), AtRate average capacity prediction raw.</summary>
		Result<void> SetAtRateAverageCapacityRaw(uint16_t value);

		/// <summary>
		/// Reads a group of learning/calibration registers used by ModelGauge learning.
		/// Internal registers.
		/// </summary>
		[[nodiscard]] Result<AlgorithmLearningParameters> GetAlgorithmLearningParameters() const;
		/// <summary>
		/// Writes a group of learning/calibration registers used by ModelGauge learning.
		/// Internal registers.
		/// </summary>
		Result<void> SetAlgorithmLearningParameters(const AlgorithmLearningParameters& value);

	private:
		I2C::Api::IDriver& m_Driver;

		Result<void> ReadRegisterRaw(RegOffset reg, uint8_t outData[2]) const;

		Result<void> WriteRegisterRaw(RegOffset reg, const uint8_t data[2]);

		template<typename T>
		[[nodiscard]] Result<T> ReadRegister(RegOffset reg) const
		{
			uint8_t data[2];
			if (auto result = ReadRegisterRaw(reg, data); !result.has_value())
			{
				return std::unexpected(result.error());
			}

			return RegUtils::Read<T, std::endian::little>(data, 0, 16);
		}

		template<typename T>
		Result<void> WriteRegister(RegOffset reg, T value)
		{
			uint8_t data[2] = {};
			RegUtils::Write<T, std::endian::little>(value, data, 0, 16);
			return WriteRegisterRaw(reg, data);
		}

		template<typename T>
		[[nodiscard]] Result<T> ReadField(RegOffset reg, size_t startBit, size_t bitCount) const
		{
			uint8_t data[2];
			if (auto result = ReadRegisterRaw(reg, data); !result.has_value())
			{
				return std::unexpected(result.error());
			}

			return RegUtils::Read<T, std::endian::little>(data, startBit, bitCount);
		}

		template<typename T>
		Result<void> WriteField(RegOffset reg, size_t startBit, size_t bitCount, T value)
		{
			uint8_t data[2];
			if (auto result = ReadRegisterRaw(reg, data); !result.has_value())
			{
				return std::unexpected(result.error());
			}

			RegUtils::Write<T, std::endian::little>(value, data, startBit, bitCount);
			return WriteRegisterRaw(reg, data);
		}
	};
};

