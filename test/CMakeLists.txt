cmake_minimum_required(VERSION 3.20)

include(FetchContent)

project(${PISUBMARINE_MODULE_NAME}.Test LANGUAGES CXX)

set(${PISUBMARINE_MODULE_NAME}.Test.Sources 
	"PiSubmarine/Max17261/DeviceTest.cpp" "PiSubmarine/Max17261/UnitsTest.cpp")

enable_testing()

# spdlog
find_package(spdlog)
find_package(GTest)

add_executable(${PISUBMARINE_MODULE_NAME}.Test ${${PISUBMARINE_MODULE_NAME}.Test.Sources})
PiSubmarineInitTarget(${PISUBMARINE_MODULE_NAME}.Test)
target_link_libraries(${PISUBMARINE_MODULE_NAME}.Test PRIVATE PiSubmarine.Max17261)
target_link_libraries(${PISUBMARINE_MODULE_NAME}.Test PRIVATE PiSubmarine.I2C.Api.Mock)

target_link_libraries(${PISUBMARINE_MODULE_NAME}.Test PRIVATE spdlog::spdlog)
target_link_libraries(${PISUBMARINE_MODULE_NAME}.Test PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(${PISUBMARINE_MODULE_NAME}.Test)