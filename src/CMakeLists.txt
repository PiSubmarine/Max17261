include("${_build_cmake_local}")

add_library(PiSubmarine.Max1726 PiSubmarine/Max17261/Device.cpp)
target_include_directories(PiSubmarine.Max1726 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../public")
PiSubmarineInitTarget(PiSubmarine.Max1726)

PiSubmarineAddDependency("https://github.com/PiSubmarine/RegUtils" "")
target_link_libraries(PiSubmarine.Max1726 PUBLIC PiSubmarine.RegUtils)

PiSubmarineAddDependency("https://github.com/PiSubmarine/I2C.Api" "")
target_link_libraries(PiSubmarine.Max1726 PUBLIC PiSubmarine.I2C.Api)

PiSubmarineAddDependency("https://github.com/PiSubmarine/Units" "")
target_link_libraries(PiSubmarine.Max1726 PUBLIC PiSubmarine.Units)