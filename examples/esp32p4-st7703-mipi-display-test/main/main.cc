#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "AppInit.h"

namespace {
constexpr char kTag[] = "main";
}

extern "C" void app_main(void)
{
    ESP_LOGI(kTag, "firmware=ydp400b001-bringup idf=%s", esp_get_idf_version());

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "nvs_flash_init failed: %s", esp_err_to_name(err));
    }

    AppInit();
}
