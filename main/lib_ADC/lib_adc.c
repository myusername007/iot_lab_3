#include "lib_adc.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

/* ESP32-S3: GPIO1 = ADC1_CH0 */
#define ADC_CHANNEL     ADC_CHANNEL_0
#define ADC_ATTEN       ADC_ATTEN_DB_12

static const char *TAG = "lib_adc";
static adc_oneshot_unit_handle_t adc1_handle = NULL;

void lib_adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &chan_cfg));
}

int lib_adc_get(void)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw));
    ESP_LOGI(TAG, "ADC1 CH0 raw: %d", raw);
    return raw;
}