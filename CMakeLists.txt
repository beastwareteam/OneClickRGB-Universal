cmake_minimum_required(VERSION 3.16)
project(OneClickRGB-Universal VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Platform Detection
# =============================================================================
if(WIN32)
    set(OCRGB_PLATFORM "WINDOWS")
    add_definitions(-DOCRGB_PLATFORM_WINDOWS)
elseif(APPLE)
    set(OCRGB_PLATFORM "MACOS")
    add_definitions(-DOCRGB_PLATFORM_MACOS)
elseif(UNIX)
    set(OCRGB_PLATFORM "LINUX")
    add_definitions(-DOCRGB_PLATFORM_LINUX)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "Target platform: ${OCRGB_PLATFORM}")

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

# Platform abstraction sources (common)
set(PLATFORM_SOURCES
    src/platform/PlatformFactory.cpp
)

# Platform-specific sources
if(WIN32)
    list(APPEND PLATFORM_SOURCES
        src/platform/windows/WindowsPlatform.cpp
        src/platform/windows/WmiSystemInfo.cpp
        src/platform/windows/WindowsDeviceEnumerator.cpp
        src/platform/windows/PawnIOSMBus.cpp
    )
elseif(APPLE)
    list(APPEND PLATFORM_SOURCES
        src/platform/macos/MacOSPlatform.cpp
        src/platform/macos/IOKitSystemInfo.cpp
        src/platform/macos/MacOSDeviceEnumerator.cpp
    )
elseif(UNIX)
    list(APPEND PLATFORM_SOURCES
        src/platform/linux/LinuxPlatform.cpp
        src/platform/linux/SysfsSystemInfo.cpp
        src/platform/linux/LinuxDeviceEnumerator.cpp
        src/platform/linux/I2CDevSMBus.cpp
    )
endif()

# =============================================================================
# Core Library
# =============================================================================
add_library(ocrgb_core STATIC
    ${CORE_SOURCES}
    ${APP_SOURCES}
    ${INTERFACE_SOURCES}
    ${PLATFORM_SOURCES}
)

target_include_directories(ocrgb_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/build/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/hidapi
)

# =============================================================================
# Platform-Specific Dependencies
# =============================================================================
if(WIN32)
    # Windows: SetupAPI, HID, WMI (wbemuuid), OLE
    target_link_libraries(ocrgb_core PUBLIC
        setupapi
        hid
        wbemuuid
        ole32
        oleaut32
    )
elseif(APPLE)
    # macOS: IOKit, CoreFoundation, HIDAPI
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)

    target_link_libraries(ocrgb_core PUBLIC
        ${IOKIT_FRAMEWORK}
        ${COREFOUNDATION_FRAMEWORK}
    )

    # Find HIDAPI (via Homebrew or pkg-config)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(HIDAPI hidapi)
        if(HIDAPI_FOUND)
            target_include_directories(ocrgb_core PUBLIC ${HIDAPI_INCLUDE_DIRS})
            target_link_libraries(ocrgb_core PUBLIC ${HIDAPI_LIBRARIES})
        endif()
    endif()
elseif(UNIX)
    # Linux: libudev, HIDAPI
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV libudev)
    pkg_check_modules(HIDAPI hidapi-libusb)

    if(UDEV_FOUND)
        target_include_directories(ocrgb_core PUBLIC ${UDEV_INCLUDE_DIRS})
        target_link_libraries(ocrgb_core PUBLIC ${UDEV_LIBRARIES})
    endif()

    if(HIDAPI_FOUND)
        target_include_directories(ocrgb_core PUBLIC ${HIDAPI_INCLUDE_DIRS})
        target_link_libraries(ocrgb_core PUBLIC ${HIDAPI_LIBRARIES})
    endif()
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
message(STATUS "  Platform:    ${OCRGB_PLATFORM}")
message(STATUS "  Build CLI:   ${BUILD_CLI}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
if(WIN32)
    message(STATUS "  SMBus:       PawnIO driver")
elseif(APPLE)
    message(STATUS "  SMBus:       Not available (HID only)")
elseif(UNIX)
    message(STATUS "  SMBus:       i2c-dev kernel module")
endif()
message(STATUS "")
