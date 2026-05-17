#pragma once
#include <stdint.h>
#include <stdbool.h>

#define DEFAULT_POWER       true
#define DEFAULT_HUE         180
#define DEFAULT_SATURATION  100
#define DEFAULT_BRIGHTNESS  25

/* ADC light level thresholds (raw 12-bit, 0-4095) */
#define LIGHT_THRESHOLD_BRIGHT   2000  /* > 2000 = світло */
#define LIGHT_THRESHOLD_DUSK     800   /* 800-2000 = сутінки */
/* < 800 = темно */

extern esp_rmaker_device_t *light_device;

void app_driver_init(void);
esp_err_t app_light_set(uint32_t hue, uint32_t saturation, uint32_t brightness);
esp_err_t app_light_set_power(bool power);
esp_err_t app_light_set_brightness(uint16_t brightness);
esp_err_t app_light_set_hue(uint16_t hue);
esp_err_t app_light_set_saturation(uint16_t saturation);