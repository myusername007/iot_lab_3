#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_console.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>

#include <app_network.h>
#include <app_insights.h>

#include "app_priv.h"
#include "lib_ADC/lib_adc.h"

static const char *TAG = "app_main";

esp_rmaker_device_t *light_device;

#define REPORTING_PERIOD  5  /* секунд, для тесту */

static TimerHandle_t sensor_timer;

/* Створення кастомного float-параметра */
static esp_rmaker_param_t *custom_param_create(const char *name, float val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(
        name, "esp.param.custom",
        esp_rmaker_float(val),
        PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
    }
    return param;
}

/* Таймер: читає ADC, визначає рівень, керує яскравістю */
static void app_sensor_update(TimerHandle_t handle)
{
    int raw = lib_adc_get();

    /* Репортуємо сирий ADC в аплікацію */
    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_type(light_device, "esp.param.custom"),
        esp_rmaker_float((float)raw));

    uint16_t brightness;
    const char *level;

    if (raw > LIGHT_THRESHOLD_BRIGHT) {
        /* Світло — не втручаємось */
        ESP_LOGI(TAG, "Light level: BRIGHT (%d), user control", raw);
        return;
    } else if (raw > LIGHT_THRESHOLD_DUSK) {
        /* Сутінки — 100% */
        brightness = 100;
        level = "DUSK";
    } else {
        /* Темно — 40% */
        brightness = 40;
        level = "DARK";
    }

    ESP_LOGI(TAG, "Light level: %s (%d), set brightness=%d", level, raw, brightness);
    app_light_set_brightness(brightness);
    esp_rmaker_param_update_and_report(
        esp_rmaker_device_get_param_by_type(light_device, ESP_RMAKER_PARAM_BRIGHTNESS),
        esp_rmaker_int(brightness));
}

/* Callback на запити з хмари */
static esp_err_t bulk_write_cb(const esp_rmaker_device_t *device,
    const esp_rmaker_param_write_req_t write_req[],
    uint8_t count, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Write via: %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    for (int i = 0; i < count; i++) {
        const esp_rmaker_param_t *param = write_req[i].param;
        esp_rmaker_param_val_t val = write_req[i].val;
        const char *param_name = esp_rmaker_param_get_name(param);

        if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
            app_light_set_power(val.val.b);
        } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
            app_light_set_brightness(val.val.i);
        } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
            app_light_set_hue(val.val.i);
        } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
            app_light_set_saturation(val.val.i);
        }
        esp_rmaker_param_update(param, val);
    }
    return ESP_OK;
}

void app_main(void)
{
    esp_rmaker_console_init();
    app_driver_init();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    app_network_init();

    esp_rmaker_config_t rainmaker_cfg = { .enable_time_sync = false };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Lightbulb");
    if (!node) {
        ESP_LOGE(TAG, "Node init failed. Aborting.");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    light_device = esp_rmaker_lightbulb_device_create("Light", NULL, DEFAULT_POWER);
    esp_rmaker_device_add_bulk_cb(light_device, bulk_write_cb, NULL);

    esp_rmaker_device_add_param(light_device,
        esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, DEFAULT_BRIGHTNESS));
    esp_rmaker_device_add_param(light_device,
        esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, DEFAULT_HUE));
    esp_rmaker_device_add_param(light_device,
        esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, DEFAULT_SATURATION));

    /* Параметр освітлення */
    esp_rmaker_device_add_param(light_device, custom_param_create("AmbientLight", 0));

    esp_rmaker_node_add_device(node, light_device);

    esp_rmaker_ota_enable_default();
    esp_rmaker_timezone_service_enable();
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();

    esp_rmaker_system_serv_config_t sys_cfg = {
        .flags = SYSTEM_SERV_FLAGS_ALL,
        .reboot_seconds = 2,
        .reset_seconds = 2,
        .reset_reboot_seconds = 2,
    };
    esp_rmaker_system_service_enable(&sys_cfg);

    app_insights_enable();
    esp_rmaker_start();

    /* Ініціалізація ADC та таймера */
    lib_adc_init();
    sensor_timer = xTimerCreate("sensor_timer",
        (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
        pdTRUE, NULL, app_sensor_update);
    if (sensor_timer) {
        xTimerStart(sensor_timer, 0);
    }

    err = app_network_set_custom_mfg_data(MFG_DATA_DEVICE_TYPE_LIGHT, MFG_DATA_DEVICE_SUBTYPE_LIGHT);
    err = app_network_start((app_network_pop_type_t)CONFIG_APP_POP_TYPE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Network start failed. Aborting.");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }
}