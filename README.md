# Lab 3: Smart Lightbulb — ESP RainMaker

**Variant 7** | ESP32-S3 | ESP-IDF v5.x | COM4

## Description

Smart lightbulb with automatic brightness control based on ambient light level,
built on ESP RainMaker framework.

## Hardware

- ESP32-S3
- WS2812 LED (GPIO 48)
- Photoresistor module (ADC1 CH0, GPIO 1)
- Boot Button (GPIO 0)

## Light Level Logic

| Level   | ADC raw     | Action                        |
|---------|-------------|-------------------------------|
| Bright  | > 2000      | User control, no auto change  |
| Dusk    | 800 – 2000  | Auto brightness = 100%        |
| Dark    | < 800       | Auto brightness = 40%         |

User can always override via app. Changes are reported to RainMaker cloud.

## Project Structure
main/
├── app_main.c       — RainMaker init, timer, light logic
├── app_driver.c     — LED + button hardware driver
├── app_priv.h       — shared defines and prototypes
└── lib_ADC/
├── lib_adc.c    — ADC oneshot driver
└── lib_adc.h    — ADC interface

## Build & Flash

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p COM4 flash monitor
```

## Provisioning

1. Flash firmware
2. Open Serial Monitor — scan QR code with ESP RainMaker app
3. Connect to 2.4GHz Wi-Fi network