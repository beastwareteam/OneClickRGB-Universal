#!/usr/bin/env python3
"""
OneClickRGB-Universal - Hardware Config Generator

Generates C++ header files from hardware_db.json for compile-time device support.
This allows the application to have built-in support for known devices without
runtime JSON parsing overhead.

Usage:
    python generate_config.py [config_dir] [output_file]

    config_dir:  Directory containing hardware_db.json (default: ../config)
    output_file: Output header file (default: ../build/generated/hardware_config.h)
"""

import json
import os
import sys
from datetime import datetime
from typing import Dict, List, Any

def parse_hex(value: str) -> int:
    """Parse hex string like '0x0B05' to integer."""
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        if value.startswith('0x') or value.startswith('0X'):
            return int(value, 16)
        return int(value)
    return 0

def mode_to_enum(mode: str) -> str:
    """Convert mode string to C++ enum value."""
    mode_map = {
        'off': 'DeviceMode::Off',
        'static': 'DeviceMode::Static',
        'breathing': 'DeviceMode::Breathing',
        'wave': 'DeviceMode::Wave',
        'spectrum': 'DeviceMode::Spectrum',
        'reactive': 'DeviceMode::Reactive',
        'color_cycle': 'DeviceMode::ColorCycle',
        'gradient': 'DeviceMode::Gradient',
        'custom': 'DeviceMode::Custom'
    }
    return mode_map.get(mode.lower(), 'DeviceMode::Static')

def type_to_enum(device_type: str) -> str:
    """Convert device type string to C++ enum value."""
    type_map = {
        'mainboard': 'DeviceType::Mainboard',
        'gpu': 'DeviceType::GPU',
        'ram': 'DeviceType::RAM',
        'keyboard': 'DeviceType::Keyboard',
        'mouse': 'DeviceType::Mouse',
        'mousepad': 'DeviceType::Mousepad',
        'headset': 'DeviceType::Headset',
        'controller': 'DeviceType::Controller',
        'ledstrip': 'DeviceType::LEDStrip',
        'fan': 'DeviceType::Fan',
        'aio': 'DeviceType::AIO',
        'case': 'DeviceType::Case',
        'other': 'DeviceType::Other'
    }
    return type_map.get(device_type.lower(), 'DeviceType::Unknown')

def protocol_to_enum(protocol) -> str:
    """Convert protocol string to C++ enum value."""
    protocol_map = {
        'hid': 'ProtocolType::HID',
        'smbus': 'ProtocolType::SMBus',
        'i2c': 'ProtocolType::I2C',
        'usb_bulk': 'ProtocolType::USB_Bulk',
        'usb_interrupt': 'ProtocolType::USB_Interrupt',
        'serial': 'ProtocolType::Serial',
        'network': 'ProtocolType::Network'
    }
    if isinstance(protocol, dict):
        protocol = protocol.get('type', 'unknown')
    if isinstance(protocol, str):
        return protocol_map.get(protocol.lower(), 'ProtocolType::Unknown')
    return 'ProtocolType::Unknown'

def generate_device_entry(device: Dict[str, Any], vendors: Dict[str, Any]) -> str:
    """Generate C++ code for a single device entry."""
    device_id = device['id']
    name = device['name']
    vendor_id = device['vendor']
    vendor_name = vendors.get(vendor_id, {}).get('name', vendor_id)
    device_type = type_to_enum(device.get('type', 'unknown'))

    # Protocol can be:
    # - A string at top level: "protocol": "hid"
    # - An object with 'bus' key: "protocol": {"bus": "hid", "type": "evision_v2"}
    # - A 'bus' field at device level
    proto_val = device.get('protocol', {})
    bus_val = device.get('bus', '')

    if isinstance(proto_val, dict):
        bus = proto_val.get('bus', bus_val)
        proto_type = proto_val.get('type', '')
        # Map protocol type names to base protocols
        if bus:
            protocol = protocol_to_enum(bus)
        elif 'hid' in proto_type.lower():
            protocol = 'ProtocolType::HID'
        elif 'smbus' in proto_type.lower():
            protocol = 'ProtocolType::SMBus'
        else:
            protocol = protocol_to_enum(proto_type)
    else:
        protocol = protocol_to_enum(proto_val)

    ids = device.get('identifiers', {})
    vid = parse_hex(ids.get('vendorId', 0))
    pid = parse_hex(ids.get('productId', 0))
    usage_page = parse_hex(ids.get('usagePage', 0))

    caps = device.get('capabilities', {})
    modes = caps.get('modes', ['static'])
    mode_list = ', '.join([mode_to_enum(m) for m in modes])
    zones = caps.get('zones', 1)
    leds_per_zone = caps.get('ledsPerZone', 1)

    proto = device.get('protocol', {})
    if isinstance(proto, str):
        proto = {}
    report_id = parse_hex(proto.get('reportId', 0))
    packet_size = proto.get('packetSize', 65)

    return f'''    {{
        "{device_id}",                          // id
        "{name}",                               // name
        "{vendor_name}",                        // vendor
        {device_type},                          // type
        {protocol},                             // protocol
        0x{vid:04X},                            // vendorId
        0x{pid:04X},                            // productId
        0x{usage_page:04X},                     // usagePage
        0x{report_id:02X},                      // reportId
        {packet_size},                          // packetSize
        {zones},                                // zones
        {leds_per_zone},                        // ledsPerZone
        {{ {mode_list} }}  // supportedModes
    }}'''

def generate_header(db: Dict[str, Any]) -> str:
    """Generate the complete C++ header file."""
    timestamp = datetime.now().isoformat()
    version = db.get('version', '1.0.0')
    vendors = db.get('vendors', {})
    devices = db.get('devices', [])

    device_entries = []
    for device in devices:
        entry = generate_device_entry(device, vendors)
        device_entries.append(entry)

    devices_array = ',\n'.join(device_entries)

    return f'''#pragma once
//=============================================================================
// OneClickRGB-Universal - Generated Hardware Configuration
//=============================================================================
// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY
// Generated: {timestamp}
// Database Version: {version}
//=============================================================================

#include "../src/core/Types.h"
#include <vector>
#include <string>

namespace OCRGB {{
namespace HardwareDB {{

//-----------------------------------------------------------------------------
// Device Definition Structure
//-----------------------------------------------------------------------------
struct DeviceDefinition {{
    const char* id;
    const char* name;
    const char* vendor;
    DeviceType type;
    ProtocolType protocol;
    uint16_t vendorId;
    uint16_t productId;
    uint16_t usagePage;
    uint8_t reportId;
    uint8_t packetSize;
    uint8_t zones;
    uint8_t ledsPerZone;
    std::vector<DeviceMode> supportedModes;
}};

//-----------------------------------------------------------------------------
// Known Devices Database
//-----------------------------------------------------------------------------
inline const std::vector<DeviceDefinition>& GetKnownDevices() {{
    static const std::vector<DeviceDefinition> devices = {{
{devices_array}
    }};
    return devices;
}}

//-----------------------------------------------------------------------------
// Device Lookup Functions
//-----------------------------------------------------------------------------
inline const DeviceDefinition* FindDeviceById(const std::string& id) {{
    for (const auto& dev : GetKnownDevices()) {{
        if (dev.id == id) return &dev;
    }}
    return nullptr;
}}

inline const DeviceDefinition* FindDeviceByVidPid(uint16_t vid, uint16_t pid) {{
    for (const auto& dev : GetKnownDevices()) {{
        if (dev.vendorId == vid && dev.productId == pid) return &dev;
    }}
    return nullptr;
}}

inline const DeviceDefinition* FindDeviceByVidPidUsage(uint16_t vid, uint16_t pid, uint16_t usagePage) {{
    for (const auto& dev : GetKnownDevices()) {{
        if (dev.vendorId == vid && dev.productId == pid) {{
            if (usagePage == 0 || dev.usagePage == 0 || dev.usagePage == usagePage) {{
                return &dev;
            }}
        }}
    }}
    return nullptr;
}}

//-----------------------------------------------------------------------------
// Statistics
//-----------------------------------------------------------------------------
inline size_t GetKnownDeviceCount() {{
    return GetKnownDevices().size();
}}

}} // namespace HardwareDB
}} // namespace OCRGB
'''

def main():
    # Parse arguments
    config_dir = sys.argv[1] if len(sys.argv) > 1 else '../config'
    output_file = sys.argv[2] if len(sys.argv) > 2 else '../build/generated/hardware_config.h'

    # Resolve paths relative to script location
    script_dir = os.path.dirname(os.path.abspath(__file__))
    config_path = os.path.join(script_dir, config_dir, 'hardware_db.json')
    output_path = os.path.join(script_dir, output_file)

    # Read database
    print(f"Reading hardware database from: {config_path}")
    with open(config_path, 'r', encoding='utf-8') as f:
        db = json.load(f)

    # Generate header
    header_content = generate_header(db)

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    # Write output
    print(f"Writing generated config to: {output_path}")
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(header_content)

    print(f"Generated config with {len(db.get('devices', []))} devices")

if __name__ == '__main__':
    main()
