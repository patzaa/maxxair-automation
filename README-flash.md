# MaxxFan Matter Controller - ESPHome

ESP32-based controller for MaxxAir/MaxxFan ceiling vent fans, designed for camper van (Sprinter) integration with Home Assistant via Ethernet.

The ESP32 emulates the physical MaxxAir remote control buttons by pulsing relay outputs through an RJ45 cable connected to the fan's control panel.

## Hardware Overview

![Schematic](schematic.png)

### Components

| Component | Part | Function |
|-----------|------|----------|
| MCU | WT32-ETH01 | ESP32 with built-in LAN8720 Ethernet |
| Relay Driver | ULN2803A | 8-channel Darlington array (3.3V GPIO → 5V relay coils) |
| Relays | 5x OMRON G6L-1P-DC5 | Emulate MaxxAir remote buttons |
| Buck Converter | TSR 0.5-2450 | 12V → 5V power supply |
| Temp/Humidity | AHT21 | I2C sensor on external connector |
| Rain Sensor | Hydreon RG-9 | UART rain gauge (external, 12V powered) |
| Fan Connector | RJ45 | 8-pin connection to MaxxAir control panel |
| UART Connector | JST B04B-XASK | 4-pin for RG-9 rain sensor (3.3V logic) |
| I2C Connector | JST B04B-XASK | 4-pin for AHT21 sensor (3.3V) |
| Power Input | MX128L 2-pin | 12V DC from vehicle battery |

### GPIO Mapping

| GPIO | Function | ULN Channel | Relay | RJ45 Pin |
|------|----------|-------------|-------|----------|
| 15 | Power | U01 → U18 | K1 | RJ7 |
| 14 | Speed + | U02 → U17 | K2 | RJ6/RJ8 |
| 12 | Speed - | U03 → U16 | K3 | RJ5/RJ8 |
| 17 | Direction | U04 → U15 | K4 | RJ5/RJ7 |
| 5 | Auto | U05 → U14 | K5 | RJ4/RJ7 |
| 2 | I2C SDA | — | — | — |
| 4 | I2C SCL | — | — | — |
| 1 | UART TX | — | — | RG-9 RX |
| 3 | UART RX | — | — | RG-9 TX |

Ethernet RMII pins (GPIO 19, 21, 22, 25, 26, 27) are used internally by the WT32-ETH01 module and are not exposed on the PCB.

### Power Distribution

```
+12V (vehicle battery)
  ├── TSR 0.5-2450 → +5V
  │     ├── ULN2803A CD+ (relay flyback diodes)
  │     ├── Relay coils (via ULN2803A outputs)
  │     └── WT32-ETH01 → 3.3V LDO (internal)
  │           ├── ESP32 + Ethernet
  │           ├── I2C bus pull-ups
  │           └── UART connector
  └── Hydreon RG-9 (5-15V supply, 3.3V UART logic)
```

## How It Works

The MaxxAir fan has a physical wall-mount remote with 5 buttons (Power, Speed+, Speed-, Direction, Auto). This controller wires relays in parallel with those buttons via the RJ45 cable.

Each "button press" is a 100ms GPIO pulse:
1. ESPHome receives command from Home Assistant
2. GPIO pin goes HIGH → ULN2803A activates → Relay closes
3. After 100ms, GPIO goes LOW → Relay opens
4. MaxxAir interprets this as a button press

Speed changes are incremental — changing from speed 2 to speed 7 requires 5 individual UP pulses. The async speed control sends one pulse per ESPHome loop cycle (non-blocking).

## Prerequisites

1. Install ESPHome:
   ```bash
   pip install esphome
   ```

2. Edit `esphome/secrets.yaml` with your credentials:
   ```yaml
   api_encryption_key: "your-base64-key-here"
   ota_password: "your-ota-password"
   ```

   Generate an encryption key:
   ```bash
   python3 -c "import secrets, base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"
   ```

## First Flash (USB-UART)

The first flash must be done via a **3.3V** USB-UART adapter connected to the UART connector (JST B04B-XASK).

**Important:** Use a 3.3V adapter only. The ESP32 is not 5V tolerant on its UART pins.

### Wiring

| UART Adapter | JST Pin | Signal |
|-------------|---------|--------|
| 3.3V | 1 | +3.3V |
| TX | 2 | UART-TX (GPIO1) |
| RX | 3 | UART-RX (GPIO3) |
| GND | 4 | GND |

### Enter flash mode

1. Connect GPIO0 to GND (Pin 24 on WT32-ETH01)
2. Power cycle or press the EN/reset button
3. Release GPIO0 after reset

### Flash

```bash
cd esphome
esphome run maxxfan.yaml
```

Select the serial port when prompted.

## Subsequent Updates (OTA)

After the first flash, updates go over Ethernet:

```bash
cd esphome
esphome run maxxfan.yaml
```

Select the network option when prompted. The device appears as `maxxfan.local`.

Alternatively, use the ESPHome Dashboard in Home Assistant for one-click OTA updates.

## Home Assistant Setup

1. Go to **Settings → Devices & Services → ESPHome**
2. The device should be auto-discovered on your network
3. Add it and enter the API encryption key from `secrets.yaml`

### Dashboard Card

1. Go to your HA dashboard → **Edit → Add Card → Manual**
2. Paste the contents of `homeassistant/lovelace-maxxfan.yaml`

Uses only built-in HA card types (no HACS required).

### Entities

| Entity | Type | Description |
|--------|------|-------------|
| `fan.maxxfan_fan` | Fan | Power on/off + speed (1-10) |
| `switch.maxxfan_cover` | Switch | Open/close fan lid |
| `switch.maxxfan_direction` | Switch | Exhaust/intake toggle |
| `sensor.maxxfan_temperature` | Sensor | AHT21 temperature |
| `sensor.maxxfan_humidity` | Sensor | AHT21 humidity |
| `sensor.maxxfan_rain_intensity` | Sensor | Hydreon RG-9 rain level |
| `sensor.maxxfan_rain_sensor_temperature` | Sensor | RG-9 internal temp |
| `binary_sensor.maxxfan_status` | Binary Sensor | Online/offline |
| `button.maxxfan_restart` | Button | Reboot ESP32 |

## Hydreon RG-9 Rain Sensor

The rain sensor connects to the UART connector (JST B04B-XASK). Since UART0 is used by the rain sensor, serial logging is disabled — logs are available over the network via ESPHome Dashboard or HA.

### Wiring

| RG-9 Wire | Connection |
|-----------|------------|
| Red (VCC) | +12V (from power connector CN3) |
| Black (GND) | GND |
| White (TX) | UART-RX (JST Pin 3 / GPIO3) |
| Green (RX) | UART-TX (JST Pin 2 / GPIO1) |

The RG-9 accepts 5-15V supply voltage but uses 3.3V UART logic — no level shifter needed.

## Project Structure

```
Maxxair-steuerung/
├── esphome/
│   ├── maxxfan.yaml              # ESPHome configuration
│   ├── secrets.yaml              # Credentials (not committed)
│   └── components/
│       └── maxxfan/
│           ├── __init__.py       # ESPHome component namespace
│           ├── output.py         # Output platform registration
│           ├── maxxfan_output.h  # C++ header
│           └── maxxfan_output.cpp # C++ relay pulse logic
├── homeassistant/
│   └── lovelace-maxxfan.yaml     # HA dashboard card
├── 1-Schematic_Fiver - WT32.json # EasyEDA schematic source
├── 1-PCB_PCB_WT32.json           # EasyEDA PCB layout source
└── README-flash.md               # This file
```
