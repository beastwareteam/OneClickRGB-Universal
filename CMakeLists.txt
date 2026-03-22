cmake_minimum_required(VERSION 3.16)
project(OneClickRGB-Universal VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Options
# =============================================================================
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_CLI "Build command-line interface" ON)

# =============================================================================
# Sources
# =============================================================================

# Core library sources
set(CORE_SOURCES
    src/core/DeviceRegistry.cpp
    src/devices/HIDDevice.cpp
    src/devices/SMBusDevice.cpp
    src/bridges/HIDBridge.cpp
    src/bridges/SMBusBridge.cpp
    src/scanner/HardwareScanner.cpp
    src/plugins/PluginFactory.cpp
    src/plugins/asus/AsusAuraController.cpp
    src/plugins/steelseries/SteelSeriesRival.cpp
    src/plugins/evision/EVisionKeyboard.cpp
    src/plugins/gskill/GSkillTridentZ5.cpp
)

# Application layer sources
set(APP_SOURCES
    src/app/config/DeviceConfiguration.cpp
    src/app/config/ConfigBundleParser.cpp
    src/app/pipeline/DevicePipeline.cpp
    src/app/effects/EffectFactory.cpp
    src/app/services/DeviceService.cpp
    src/app/services/ProvisioningService.cpp
    src/app/services/ProfileResolver.cpp
    src/app/fingerprint/MachineFingerprint.cpp
)

# Simple interface sources
set(INTERFACE_SOURCES
    src/OneClickRGB.cpp
)

# =============================================================================
# Core Library
# =============================================================================
add_library(ocrgb_core STATIC ${CORE_SOURCES} ${APP_SOURCES} ${INTERFACE_SOURCES})

target_include_directories(ocrgb_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/build/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/hidapi
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(ocrgb_core PUBLIC setupapi hid)
endif()

# =============================================================================
# CLI Executable
# =============================================================================
if(BUILD_CLI)
    add_executable(oneclickrgb src/main.cpp)
    target_link_libraries(oneclickrgb PRIVATE ocrgb_core)

    # Install
    install(TARGETS oneclickrgb RUNTIME DESTINATION bin)
endif()

# =============================================================================
# Tests
# =============================================================================
if(BUILD_TESTS)
    enable_testing()

    add_executable(test_runner
        tests/test_main.cpp
        # Note: test_main.cpp includes all test_*.cpp files
    )

    target_include_directories(test_runner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(test_runner PRIVATE ocrgb_core)

    add_test(NAME unit_tests COMMAND test_runner)
endif()

# =============================================================================
# Summary
# =============================================================================
message(STATUS "")
message(STATUS "OneClickRGB-Universal ${PROJECT_VERSION}")
message(STATUS "  Build CLI:   ${BUILD_CLI}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "")
