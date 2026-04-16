#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <span>
#include "PiSubmarine/Error/Api/MakeError.h"
#include "PiSubmarine/Max17261/Device.h"
#include "PiSubmarine/I2C/Api/IDriverMock.h"

namespace PiSubmarine::Max17261
{
	using ::testing::_;
	using ::testing::Invoke;
	using ::testing::Return;
	using ErrorCondition = PiSubmarine::Error::Api::ErrorCondition;

	TEST(DeviceTest, SetsAndGetsVoltageAlertThresholdRaw)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, Write(Device::Address, _))
			.WillOnce(Invoke([](uint8_t, std::span<const uint8_t> tx)
			{
				EXPECT_EQ(tx.size(), 3u);
				EXPECT_EQ(tx[0], static_cast<uint8_t>(RegOffset::VAlrtTh));
				EXPECT_EQ(tx[1], 0x55);
				EXPECT_EQ(tx[2], 0xAA);
				return PiSubmarine::Error::Api::Result<void>{};
			}));

		auto setResult = device.SetVoltageAlertThresholdRaw({ .Maximum = 0xAA, .Minimum = 0x55 });
		EXPECT_TRUE(setResult.has_value());

		EXPECT_CALL(driver, WriteRead(Device::Address, _, _))
			.WillOnce(Invoke([](uint8_t, std::span<const uint8_t> tx, std::span<uint8_t> rx)
			{
				EXPECT_EQ(tx.size(), 1u);
				EXPECT_EQ(tx[0], static_cast<uint8_t>(RegOffset::VAlrtTh));
				EXPECT_EQ(rx.size(), 2u);
				rx[0] = 0x55;
				rx[1] = 0xAA;
				return PiSubmarine::Error::Api::Result<void>{};
			}));

		auto getResult = device.GetVoltageAlertThresholdRaw();
		ASSERT_TRUE(getResult.has_value());
		EXPECT_EQ(getResult->Maximum, 0xAA);
		EXPECT_EQ(getResult->Minimum, 0x55);
	}

	TEST(DeviceTest, GetsInstantVoltageFromRegisterRaw)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, WriteRead(Device::Address, _, _))
			.WillOnce(Invoke([](uint8_t, std::span<const uint8_t> tx, std::span<uint8_t> rx)
			{
				EXPECT_EQ(tx[0], static_cast<uint8_t>(RegOffset::VCell));
				rx[0] = 0x10;
				rx[1] = 0x27;
				return PiSubmarine::Error::Api::Result<void>{};
			}));

		auto voltageResult = device.GetInstantVoltage();
		ASSERT_TRUE(voltageResult.has_value());
		EXPECT_GT(voltageResult->GetMicroVolts(), 0u);
	}

	TEST(DeviceTest, PropagatesWriteError)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, Write(Device::Address, _)).WillOnce(Return(std::unexpected(
			PiSubmarine::Error::Api::MakeError(ErrorCondition::CommunicationError))));

		auto result = device.SetCycleCount(7);
		ASSERT_FALSE(result.has_value());
		EXPECT_EQ(result.error().Condition, ErrorCondition::CommunicationError);
	}

	TEST(DeviceTest, PropagatesWriteReadError)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, WriteRead(Device::Address, _, _)).WillOnce(Return(std::unexpected(
			PiSubmarine::Error::Api::MakeError(ErrorCondition::CommunicationError))));

		auto result = device.GetCycleCount();
		ASSERT_FALSE(result.has_value());
		EXPECT_EQ(result.error().Condition, ErrorCondition::CommunicationError);
	}

	TEST(DeviceTest, AlgorithmLearningParametersSerializationRoundtrip)
	{
		AlgorithmLearningParameters original{};
		original.LearningConfig.Raw = 0x1111;
		original.FilterConfig.Raw = 0x2222;
		original.RelaxConfig.Raw = 0x3333;
		original.MiscConfig.Raw = 0x4444;
		original.TempGain.Raw = 0x5555;
		original.TempOffset.Raw = 0x6666;
		original.ChargeCurrentGain.Raw = 0x7777;
		original.ChargeCurrentOffset.Raw = 0x8888;
		original.TemperatureCompensationBaselineRaw = 0x9999;
		original.TemperatureCompensationCoefficientRaw = 0xAAAA;
		original.CapacityDeltaAccumulator.Raw = 0xBBBB;
		original.PowerDeltaAccumulator.Raw = 0xCCCC;

		auto bytes = original.Serialize();
		auto restored = AlgorithmLearningParameters::Deserialize(bytes);

		EXPECT_EQ(restored.LearningConfig.Raw, 0x1111);
		EXPECT_EQ(restored.FilterConfig.Raw, 0x2222);
		EXPECT_EQ(restored.RelaxConfig.Raw, 0x3333);
		EXPECT_EQ(restored.MiscConfig.Raw, 0x4444);
		EXPECT_EQ(restored.TempGain.Raw, 0x5555);
		EXPECT_EQ(restored.TempOffset.Raw, 0x6666);
		EXPECT_EQ(restored.ChargeCurrentGain.Raw, 0x7777);
		EXPECT_EQ(restored.ChargeCurrentOffset.Raw, 0x8888);
		EXPECT_EQ(restored.TemperatureCompensationBaselineRaw, 0x9999);
		EXPECT_EQ(restored.TemperatureCompensationCoefficientRaw, 0xAAAA);
		EXPECT_EQ(restored.CapacityDeltaAccumulator.Raw, 0xBBBB);
		EXPECT_EQ(restored.PowerDeltaAccumulator.Raw, 0xCCCC);
	}

	TEST(DeviceTest, AlgorithmLearningParametersSetWritesAllRegisters)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);
		AlgorithmLearningParameters p{};
		p.LearningConfig.Raw = 1;
		p.FilterConfig.Raw = 2;
		p.RelaxConfig.Raw = 3;
		p.MiscConfig.Raw = 4;
		p.TempGain.Raw = 5;
		p.TempOffset.Raw = 6;
		p.ChargeCurrentGain.Raw = 7;
		p.ChargeCurrentOffset.Raw = 8;
		p.TemperatureCompensationBaselineRaw = 9;
		p.TemperatureCompensationCoefficientRaw = 10;
		p.CapacityDeltaAccumulator.Raw = 11;
		p.PowerDeltaAccumulator.Raw = 12;

		EXPECT_CALL(driver, Write(Device::Address, _)).Times(12).WillRepeatedly(Return(PiSubmarine::Error::Api::Result<void>{}));

		auto result = device.SetAlgorithmLearningParameters(p);
		EXPECT_TRUE(result.has_value());
	}

	TEST(DeviceTest, AlgorithmLearningParametersGetReadsAllRegisters)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, WriteRead(Device::Address, _, _))
			.Times(12)
			.WillRepeatedly(Invoke([](uint8_t, std::span<const uint8_t> tx, std::span<uint8_t> rx)
			{
				const uint16_t value = static_cast<uint16_t>(tx[0]) * 3u;
				rx[0] = static_cast<uint8_t>(value & 0xFF);
				rx[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
				return PiSubmarine::Error::Api::Result<void>{};
			}));

		auto result = device.GetAlgorithmLearningParameters();
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->LearningConfig.Raw, static_cast<uint16_t>(static_cast<uint8_t>(RegOffset::LearnCfg) * 3u));
		EXPECT_EQ(result->PowerDeltaAccumulator.Raw, static_cast<uint16_t>(static_cast<uint8_t>(RegOffset::dPAcc) * 3u));
	}
}

