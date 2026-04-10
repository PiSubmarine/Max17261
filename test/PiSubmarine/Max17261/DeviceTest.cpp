#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "PiSubmarine/Max17261/Device.h"
#include "PiSubmarine/I2C/Api/IDriverMock.h"

namespace PiSubmarine::Max17261
{
	using ::testing::_;
	using ::testing::Invoke;
	using ::testing::Return;

	TEST(DeviceTest, SetsAndGetsVoltageAlertThresholdRaw)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, Write(Device::Address, _, 3))
			.WillOnce(Invoke([](uint8_t, uint8_t* tx, std::size_t len)
			{
				EXPECT_EQ(len, 3u);
				EXPECT_EQ(tx[0], static_cast<uint8_t>(RegOffset::VAlrtTh));
				EXPECT_EQ(tx[1], 0x55);
				EXPECT_EQ(tx[2], 0xAA);
				return true;
			}));

		auto setResult = device.SetVoltageAlertThresholdRaw({ .Maximum = 0xAA, .Minimum = 0x55 });
		EXPECT_EQ(setResult, DeviceError::None);

		EXPECT_CALL(driver, WriteRead(Device::Address, _, 1, _, 2))
			.WillOnce(Invoke([](uint8_t, uint8_t* tx, std::size_t txLen, uint8_t* rx, std::size_t rxLen)
			{
				EXPECT_EQ(txLen, 1u);
				EXPECT_EQ(tx[0], static_cast<uint8_t>(RegOffset::VAlrtTh));
				EXPECT_EQ(rxLen, 2u);
				rx[0] = 0x55;
				rx[1] = 0xAA;
				return true;
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

		EXPECT_CALL(driver, WriteRead(Device::Address, _, 1, _, 2))
			.WillOnce(Invoke([](uint8_t, uint8_t* tx, std::size_t, uint8_t* rx, std::size_t)
			{
				EXPECT_EQ(tx[0], static_cast<uint8_t>(RegOffset::VCell));
				rx[0] = 0x10;
				rx[1] = 0x27;
				return true;
			}));

		auto voltageResult = device.GetInstantVoltage();
		ASSERT_TRUE(voltageResult.has_value());
		EXPECT_GT(voltageResult->GetMicroVolts(), 0u);
	}

	TEST(DeviceTest, PropagatesWriteError)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, Write(Device::Address, _, 3)).WillOnce(Return(false));

		auto result = device.SetCycleCount(7);
		EXPECT_EQ(result, DeviceError::WriteFailed);
	}

	TEST(DeviceTest, PropagatesWriteReadError)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, WriteRead(Device::Address, _, 1, _, 2)).WillOnce(Return(false));

		auto result = device.GetCycleCount();
		ASSERT_FALSE(result.has_value());
		EXPECT_EQ(result.error(), DeviceError::WriteReadFailed);
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

		EXPECT_CALL(driver, Write(Device::Address, _, 3)).Times(12).WillRepeatedly(Return(true));

		auto result = device.SetAlgorithmLearningParameters(p);
		EXPECT_EQ(result, DeviceError::None);
	}

	TEST(DeviceTest, AlgorithmLearningParametersGetReadsAllRegisters)
	{
		PiSubmarine::I2C::Api::IDriverMock driver;
		Device device(driver);

		EXPECT_CALL(driver, WriteRead(Device::Address, _, 1, _, 2))
			.Times(12)
			.WillRepeatedly(Invoke([](uint8_t, uint8_t* tx, std::size_t, uint8_t* rx, std::size_t)
			{
				const uint16_t value = static_cast<uint16_t>(tx[0]) * 3u;
				rx[0] = static_cast<uint8_t>(value & 0xFF);
				rx[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
				return true;
			}));

		auto result = device.GetAlgorithmLearningParameters();
		ASSERT_TRUE(result.has_value());
		EXPECT_EQ(result->LearningConfig.Raw, static_cast<uint16_t>(static_cast<uint8_t>(RegOffset::LearnCfg) * 3u));
		EXPECT_EQ(result->PowerDeltaAccumulator.Raw, static_cast<uint16_t>(static_cast<uint8_t>(RegOffset::dPAcc) * 3u));
	}
}
