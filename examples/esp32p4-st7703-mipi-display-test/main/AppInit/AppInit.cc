#include "AppInit.h"

#include "DisplayTest.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lcd_init.h"

static const char* TAG = "AppInit";

extern "C" void AppInit(void)
{
    ESP_LOGI(TAG, "ESP32-P4 module + YDP400B001-V1 display bring-up");

    if (!LCDInit()) {
        ESP_LOGE(TAG, "LCD init failed at stage %s, error=%d", LCDGetStatus(), LCDGetLastError());
        return;
    }

    if (!lvgl_port_lock(5000)) {
        ESP_LOGE(TAG, "LVGL lock timeout");
        return;
    }
    DisplayTest_Create();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "App ready, lcd_status=%s", LCDGetStatus());
}
