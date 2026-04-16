include("${_build_cmake_local}")

add_library(PiSubmarine.Max17261 PiSubmarine/Max17261/Device.cpp)
target_include_directories(PiSubmarine.Max17261 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../public")
PiSubmarineInitTarget(PiSubmarine.Max17261)

PiSubmarineAddDependency("https://github.com/PiSubmarine/RegUtils" "")
target_link_libraries(PiSubmarine.Max17261 PUBLIC PiSubmarine.RegUtils)

PiSubmarineAddDependency("https://github.com/PiSubmarine/I2C.Api" "")
target_link_libraries(PiSubmarine.Max17261 PUBLIC PiSubmarine.I2C.Api)

PiSubmarineAddDependency("https://github.com/PiSubmarine/Error.Api.git" "")
target_link_libraries(PiSubmarine.Max17261 PUBLIC PiSubmarine.Error.Api)

PiSubmarineAddDependency("https://github.com/PiSubmarine/Units" "")
target_link_libraries(PiSubmarine.Max17261 PUBLIC PiSubmarine.Units)
