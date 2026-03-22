# Raspberry Pi Support

OneClickRGB-Universal unterstützt Raspberry Pi (alle Modelle mit Pi OS).

---

## Schnellstart

```bash
# 1. Abhängigkeiten installieren
sudo apt update
sudo apt install build-essential cmake libhidapi-dev libudev-dev

# 2. I2C aktivieren (für SMBus/RAM)
sudo raspi-config
# → Interface Options → I2C → Enable
# → Reboot

# 3. Benutzer zur i2c-Gruppe hinzufügen
sudo usermod -aG i2c $USER
# Neu einloggen oder: newgrp i2c

# 4. Build
git clone https://github.com/beastwareteam/OneClickRGB-Universal.git
cd OneClickRGB-Universal
mkdir build && cd build
cmake ..
make -j4

# 5. Test
./oneclickrgb --help
```

---

## Unterstützte Funktionen

| Feature | Pi Support | Anmerkung |
|---------|------------|-----------|
| HID-Geräte (USB) | ✓ | Keyboards, Mäuse, etc. |
| SMBus (I2C) | ✓ | RGB-RAM, falls angeschlossen |
| GPIO RGB-LEDs | ✓ | WS2812B, APA102, PWM |
| Hardware-Fingerprint | ✓ | /proc/cpuinfo, device-tree |
| Auto-Provisioning | ✓ | Vollständig |

---

## GPIO RGB-LEDs

### Unterstützte LED-Typen

| Typ | Protokoll | GPIO | Bibliothek |
|-----|-----------|------|------------|
| WS2812B (NeoPixel) | 1-Wire | GPIO18 (PWM) | rpi_ws281x |
| WS2811 | 1-Wire | GPIO18 | rpi_ws281x |
| SK6812 (RGBW) | 1-Wire | GPIO18 | rpi_ws281x |
| APA102 (DotStar) | SPI | GPIO10/11 | SPI |
| Einfache RGB-LED | PWM | GPIO12/13/19 | pigpio |

### Verkabelung WS2812B

```
Raspberry Pi              WS2812B Strip
-----------               -------------
GPIO18 (Pin 12) ────────► DIN (Data In)
5V     (Pin 2/4) ───────► 5V
GND    (Pin 6) ─────────► GND

WICHTIG: Pegelwandler 3.3V→5V empfohlen für zuverlässige Signale
```

### Verkabelung APA102 (SPI)

```
Raspberry Pi              APA102 Strip
-----------               ------------
GPIO10 MOSI (Pin 19) ───► DI (Data In)
GPIO11 SCLK (Pin 23) ───► CI (Clock In)
5V     (Pin 2/4) ───────► 5V
GND    (Pin 6) ─────────► GND
```

### Verkabelung PWM RGB-LED

```
Raspberry Pi              RGB LED (Common Cathode)
-----------               -----------------------
GPIO12 (Pin 32) ─[330Ω]─► R (Red)
GPIO13 (Pin 33) ─[330Ω]─► G (Green)
GPIO19 (Pin 35) ─[330Ω]─► B (Blue)
GND    (Pin 6) ─────────► GND (Common)
```

---

## Installation der GPIO-Bibliotheken

### WS2812B (rpi_ws281x)

```bash
# Bibliothek installieren
sudo apt install python3-dev swig
git clone https://github.com/jgarff/rpi_ws281x.git
cd rpi_ws281x
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig
```

### SPI aktivieren (für APA102)

```bash
sudo raspi-config
# → Interface Options → SPI → Enable

# Verify
ls /dev/spi*
# Sollte /dev/spidev0.0 zeigen
```

### pigpio (für PWM)

```bash
sudo apt install pigpio python3-pigpio
sudo systemctl enable pigpiod
sudo systemctl start pigpiod
```

---

## Konfiguration

### hardware_db.json Erweiterung

```json
{
  "devices": [
    {
      "id": "rpi-ws2812b",
      "name": "WS2812B LED Strip",
      "vendor": "Generic",
      "type": "GPIO",
      "protocol": "ws2812",
      "config": {
        "gpio_pin": 18,
        "led_count": 60,
        "led_type": "WS2812B",
        "color_order": "GRB",
        "brightness": 255
      }
    },
    {
      "id": "rpi-apa102",
      "name": "APA102 LED Strip",
      "vendor": "Generic",
      "type": "GPIO",
      "protocol": "apa102",
      "config": {
        "spi_device": "/dev/spidev0.0",
        "led_count": 30,
        "brightness": 31
      }
    },
    {
      "id": "rpi-pwm-led",
      "name": "PWM RGB LED",
      "vendor": "Generic",
      "type": "GPIO",
      "protocol": "pwm",
      "config": {
        "gpio_red": 12,
        "gpio_green": 13,
        "gpio_blue": 19,
        "pwm_frequency": 800
      }
    }
  ]
}
```

---

## Berechtigungen

### Ohne sudo ausführen

```bash
# GPIO-Gruppe
sudo usermod -aG gpio $USER

# I2C-Gruppe (für SMBus)
sudo usermod -aG i2c $USER

# SPI-Gruppe (für APA102)
sudo usermod -aG spi $USER

# Video-Gruppe (für PWM DMA)
sudo usermod -aG video $USER

# Neu einloggen erforderlich
```

### udev-Regeln für HID

```bash
# /etc/udev/rules.d/99-oneclickrgb.rules
sudo tee /etc/udev/rules.d/99-oneclickrgb.rules << 'EOF'
# ASUS Aura
SUBSYSTEM=="usb", ATTR{idVendor}=="0b05", MODE="0666"
# SteelSeries
SUBSYSTEM=="usb", ATTR{idVendor}=="1038", MODE="0666"
# EVision
SUBSYSTEM=="usb", ATTR{idVendor}=="0c45", MODE="0666"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## Raspberry Pi Fingerprint

Der Hardware-Fingerprint auf Pi enthält:

```cpp
// /proc/cpuinfo
Model           : Raspberry Pi 4 Model B Rev 1.4
Serial          : 10000000abcd1234
Hardware        : BCM2711
Revision        : d03114

// /proc/device-tree/model
Raspberry Pi 4 Model B Rev 1.4
```

### Beispiel-Fingerprint

```json
{
  "machineId": "rpi-10000000abcd1234",
  "platform": "linux",
  "arch": "aarch64",
  "model": "Raspberry Pi 4 Model B",
  "revision": "d03114",
  "serial": "10000000abcd1234",
  "memory_mb": 4096
}
```

---

## Autostart (systemd)

```bash
# Service-Datei erstellen
sudo tee /etc/systemd/system/oneclickrgb.service << 'EOF'
[Unit]
Description=OneClickRGB LED Controller
After=network.target

[Service]
Type=simple
User=pi
ExecStart=/home/pi/OneClickRGB-Universal/build/oneclickrgb provision --auto
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

# Aktivieren
sudo systemctl daemon-reload
sudo systemctl enable oneclickrgb
sudo systemctl start oneclickrgb

# Status prüfen
sudo systemctl status oneclickrgb
```

---

## Troubleshooting

### "Can't open /dev/mem" (WS2812B)

```bash
# Root-Rechte erforderlich für DMA
sudo ./oneclickrgb set 255 0 0

# Oder: setcap für Binary
sudo setcap cap_sys_rawio+ep ./oneclickrgb
```

### I2C nicht gefunden

```bash
# Prüfen ob aktiviert
ls /dev/i2c*

# Manuell laden
sudo modprobe i2c-dev
sudo modprobe i2c-bcm2835

# Permanent aktivieren
echo "i2c-dev" | sudo tee -a /etc/modules
```

### SPI nicht gefunden

```bash
# Prüfen
ls /dev/spi*

# In /boot/config.txt
dtparam=spi=on
```

### Niedrige Bildrate bei WS2812B

```bash
# Audio deaktivieren (teilt PWM-Hardware)
# In /boot/config.txt:
dtparam=audio=off
```

---

## Performance-Tipps

| Einstellung | Empfehlung |
|-------------|------------|
| LED-Count | Max. 300 für flüssige 60fps |
| PWM-Frequenz | 800kHz für WS2812B |
| SPI-Speed | 8MHz für APA102 |
| CPU Governor | performance |

```bash
# CPU auf Performance setzen
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

---

## Beispiele

### Einfacher Test

```bash
# Alle LEDs rot
./oneclickrgb set 255 0 0

# Rainbow-Modus
./oneclickrgb mode rainbow

# Helligkeit 50%
./oneclickrgb brightness 50
```

### Python-Integration

```python
import subprocess

def set_color(r, g, b):
    subprocess.run(['./oneclickrgb', 'set', str(r), str(g), str(b)])

def set_mode(mode):
    subprocess.run(['./oneclickrgb', 'mode', mode])

# Beispiel
set_color(0, 255, 128)
set_mode('breathing')
```

---

*Getestet auf: Raspberry Pi 4B, Pi 3B+, Pi Zero 2W mit Pi OS Bookworm*
