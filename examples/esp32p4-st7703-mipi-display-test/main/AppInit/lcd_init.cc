#include "lcd_init.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_st7703.h"
#include "lvgl.h"

static const char* TAG = "LCD";

/* YDP400B001-V1: 4.0 inch IPS, 720x720, ST7703, 2-lane MIPI-DSI. */
#define LCD_H_RES 720
#define LCD_V_RES 720

#define LCD_DSI_LANE_NUM 2
#define LCD_DSI_LANE_BITRATE_MBPS 600
#define LCD_DPI_CLOCK_MHZ 46

#define LCD_MIPI_PHY_LDO_CHAN 3
#define LCD_MIPI_PHY_LDO_MV 2500

/* ESP32P4 module baseboard V1.5 netlist-derived pins. */
#define LCD_PIN_RST GPIO_NUM_5
#define LCD_PIN_TE GPIO_NUM_4
#define LCD_PIN_BL_EN GPIO_NUM_20

#define BOOT_LOGO_W 400
#define BOOT_LOGO_H 183

extern const uint8_t boot_logo_rgb_start[] asm("_binary_boot_logo_rgb_start");
extern const uint8_t boot_logo_rgb_end[] asm("_binary_boot_logo_rgb_end");

#if LV_COLOR_DEPTH == 16
#define LCD_PIXEL_FORMAT LCD_COLOR_FMT_RGB565
#else
#define LCD_PIXEL_FORMAT LCD_COLOR_FMT_RGB888
#endif

static esp_lcd_panel_handle_t s_panel;
static esp_lcd_panel_io_handle_t s_io;
static esp_ldo_channel_handle_t s_mipi_ldo;
static lv_display_t* s_lvgl_disp;
static volatile const char* s_status = "not_started";
static volatile int s_last_error;

/* Working sequence from the panel test fixture: ST7703_BOE3.95_testOK.c */
static const st7703_lcd_init_cmd_t s_lcd_init_cmds[] = {
    {0x11, NULL, 0, 1000},
    {0xB9, (uint8_t[]){0xF1, 0x12, 0x83}, 3, 0},
    {0xB1, (uint8_t[]){0x00, 0x00, 0x00, 0xDA, 0x80}, 5, 0},
    {0xB2, (uint8_t[]){0x3C, 0x12, 0x30}, 3, 0},
    {0xB3, (uint8_t[]){0x10, 0x10, 0x28, 0x28, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00}, 10, 0},
    {0xB4, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x0A, 0x0A}, 2, 0},
    {0xB6, (uint8_t[]){0x97, 0x97}, 2, 0},
    {0xB8, (uint8_t[]){0x26, 0x22, 0xF0, 0x63}, 4, 0},
    {0xBA, (uint8_t[]){0x31, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x44, 0x25, 0x00, 0x90, 0x0A, 0x00,
                       0x00, 0x01, 0x4F, 0x01, 0x00, 0x00, 0x37}, 27, 0},
    {0xBC, (uint8_t[]){0x47}, 1, 0},
    {0xBF, (uint8_t[]){0x02, 0x11, 0x00}, 3, 0},
    {0xC0, (uint8_t[]){0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x12, 0x70, 0x00}, 9, 0},
    {0xC1, (uint8_t[]){0x43, 0x00, 0x32, 0x32, 0x77, 0xC1, 0xFF, 0xFF, 0xCC, 0xCC,
                       0x77, 0x77}, 12, 0},
    {0xC6, (uint8_t[]){0x82, 0x00, 0xBF, 0xFF, 0x00, 0xFF}, 6, 0},
    {0xC7, (uint8_t[]){0xB8, 0x00, 0x0A, 0x00, 0x00, 0x00}, 6, 0},
    {0xC8, (uint8_t[]){0x10, 0x40, 0x1E, 0x02}, 4, 0},
    {0xCC, (uint8_t[]){0x0B}, 1, 0},
    {0xE0, (uint8_t[]){0x00, 0x0B, 0x10, 0x2C, 0x3D, 0x3F, 0x42, 0x3A, 0x07, 0x0D,
                       0x0F, 0x13, 0x15, 0x13, 0x14, 0x0F, 0x16, 0x00, 0x0B, 0x10,
                       0x2C, 0x3D, 0x3F, 0x42, 0x3A, 0x07, 0x0D, 0x0F, 0x13, 0x15,
                       0x13, 0x14, 0x0F, 0x16}, 34, 0},
    {0xE3, (uint8_t[]){0x03, 0x03, 0x03, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
                       0xFF, 0x00, 0xC0, 0x10}, 14, 0},
    {0xE9, (uint8_t[]){0xC8, 0x10, 0x0A, 0x00, 0x00, 0x80, 0x81, 0x12, 0x31, 0x23,
                       0x4F, 0x86, 0xA0, 0x00, 0x47, 0x08, 0x00, 0x00, 0x0C, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x98, 0x02,
                       0x8B, 0xAF, 0x46, 0x02, 0x88, 0x88, 0x88, 0x88, 0x88, 0x98,
                       0x13, 0x8B, 0xAF, 0x57, 0x13, 0x88, 0x88, 0x88, 0x88, 0x88,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00}, 63, 0},
    {0xEA, (uint8_t[]){0x97, 0x0C, 0x09, 0x09, 0x09, 0x78, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x9F, 0x31, 0x8B, 0xA8, 0x31, 0x75, 0x88, 0x88,
                       0x88, 0x88, 0x88, 0x9F, 0x20, 0x8B, 0xA8, 0x20, 0x64, 0x88,
                       0x88, 0x88, 0x88, 0x88, 0x23, 0x00, 0x00, 0x02, 0xB0, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x40, 0x80, 0x81, 0x00, 0x00, 0x00,
                       0x00}, 61, 0},
    {0xEF, (uint8_t[]){0xFF, 0xFF, 0x01}, 3, 0},
    {0x3A, (uint8_t[]){0x77}, 1, 0},
    {0x29, NULL, 0, 150},
};

static void InitBacklightPin(void)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << LCD_PIN_BL_EN;
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_BL_EN, 0));
}

static void InitTePin(void)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << LCD_PIN_TE;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

static void EnableMipiPhyPower(void)
{
    esp_ldo_channel_config_t cfg = {};
    cfg.chan_id = LCD_MIPI_PHY_LDO_CHAN;
    cfg.voltage_mv = LCD_MIPI_PHY_LDO_MV;
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&cfg, &s_mipi_ldo));
    ESP_LOGI(TAG, "MIPI DSI PHY LDO%d enabled at %d mV", LCD_MIPI_PHY_LDO_CHAN, LCD_MIPI_PHY_LDO_MV);
}

void LCDSetBacklight(bool on)
{
    gpio_set_level(LCD_PIN_BL_EN, on ? 1 : 0);
}

static void ResetPanel(void)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << LCD_PIN_RST;
    cfg.mode = GPIO_MODE_OUTPUT;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&cfg));

    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_RST, 0));
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_RST, 1));
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_LOGI(TAG, "panel reset released (GPIO%d)", LCD_PIN_RST);
}

static void DrawSolidColor(uint32_t rgb, uint32_t delay_ms)
{
    const size_t pixels = (size_t)LCD_H_RES * LCD_V_RES;
    uint8_t* fb = (uint8_t*)heap_caps_malloc(pixels * 3, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (fb == NULL) {
        ESP_LOGW(TAG, "solid color buffer alloc failed");
        return;
    }
    const uint8_t r = (rgb >> 16) & 0xFF;
    const uint8_t g = (rgb >> 8) & 0xFF;
    const uint8_t b = rgb & 0xFF;
    for (size_t i = 0; i < pixels; ++i) {
        fb[i * 3 + 0] = b;
        fb[i * 3 + 1] = g;
        fb[i * 3 + 2] = r;
    }
    esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, LCD_H_RES, LCD_V_RES, fb);
    ESP_LOGI(TAG, "solid color 0x%06lX draw %s", (unsigned long)rgb, esp_err_to_name(err));
    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    free(fb);
}

static void DrawSolidColorTest(void)
{
    DrawSolidColor(0xFF0000, 2000);  // red
    DrawSolidColor(0x00FF00, 2000);  // green
    DrawSolidColor(0x0000FF, 2000);  // blue
    DrawSolidColor(0xFFFFFF, 2000);  // white
}

static void ShowBootLogo(void)
{
    const size_t logo_size = (size_t)BOOT_LOGO_W * BOOT_LOGO_H * 3;
    const size_t embedded_size = (size_t)(boot_logo_rgb_end - boot_logo_rgb_start);
    if (embedded_size != logo_size) {
        ESP_LOGW(TAG, "boot logo size mismatch: %u != %u", (unsigned)embedded_size, (unsigned)logo_size);
        return;
    }

    /* Full-screen white background, then the pre-scaled logo centered. */
    DrawSolidColor(0xFFFFFF, 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t* logo = (uint8_t*)heap_caps_malloc(logo_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (logo == NULL) {
        ESP_LOGW(TAG, "boot logo buffer alloc failed");
        return;
    }
    memcpy(logo, boot_logo_rgb_start, logo_size);

    const int x = (LCD_H_RES - BOOT_LOGO_W) / 2;
    const int y = (LCD_V_RES - BOOT_LOGO_H) / 2;
    esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, x, y, x + BOOT_LOGO_W, y + BOOT_LOGO_H, logo);
    ESP_LOGI(TAG, "boot logo %dx%d at (%d,%d) draw %s", BOOT_LOGO_W, BOOT_LOGO_H, x, y,
             esp_err_to_name(err));
    vTaskDelay(pdMS_TO_TICKS(2000));
    free(logo);
}

static esp_err_t InitLvgl(void)
{
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init failed");

    lvgl_port_display_cfg_t disp_cfg = {};
    disp_cfg.io_handle = s_io;
    disp_cfg.panel_handle = s_panel;
    disp_cfg.buffer_size = LCD_H_RES * LCD_V_RES;
    disp_cfg.double_buffer = true;
    disp_cfg.hres = LCD_H_RES;
    disp_cfg.vres = LCD_V_RES;
    disp_cfg.monochrome = false;
    disp_cfg.rotation.swap_xy = false;
    disp_cfg.rotation.mirror_x = false;
    disp_cfg.rotation.mirror_y = false;
#if LVGL_VERSION_MAJOR >= 9
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB888;
#endif
    disp_cfg.flags.buff_dma = false;
    disp_cfg.flags.buff_spiram = true;
    disp_cfg.flags.sw_rotate = true;
#if LVGL_VERSION_MAJOR >= 9
    disp_cfg.flags.swap_bytes = false;
#endif
    disp_cfg.flags.full_refresh = true;
    disp_cfg.flags.direct_mode = false;

    lvgl_port_display_dsi_cfg_t dsi_cfg = {};
    dsi_cfg.flags.avoid_tearing = true;

    s_lvgl_disp = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    ESP_RETURN_ON_FALSE(s_lvgl_disp != NULL, ESP_FAIL, TAG, "lvgl_port_add_disp_dsi failed");
    ESP_LOGI(TAG, "LVGL DSI tear-free: 2 frame buffers, full refresh, sync to panel VSYNC/TE (16.6ms)");
    return ESP_OK;
}

bool LCDInit(void)
{
    s_last_error = 0;
    s_status = "backlight_pin";
    InitBacklightPin();
    InitTePin();

    s_status = "mipi_phy";
    EnableMipiPhyPower();
    ResetPanel();

    esp_lcd_dsi_bus_handle_t dsi_bus = NULL;
    esp_lcd_dsi_bus_config_t bus_cfg = {};
    bus_cfg.bus_id = 0;
    bus_cfg.num_data_lanes = LCD_DSI_LANE_NUM;
    bus_cfg.lane_bit_rate_mbps = LCD_DSI_LANE_BITRATE_MBPS;
    bus_cfg.flags.clock_lane_force_hs = 0;

    s_status = "dsi_bus";
    esp_err_t err = esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus);
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "esp_lcd_new_dsi_bus failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "DSI bus ready: %d lanes, %d Mbps/lane", LCD_DSI_LANE_NUM, LCD_DSI_LANE_BITRATE_MBPS);

    esp_lcd_dbi_io_config_t dbi_cfg = {};
    dbi_cfg.virtual_channel = 0;
    dbi_cfg.lcd_cmd_bits = 8;
    dbi_cfg.lcd_param_bits = 8;

    s_status = "dbi_io";
    err = esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &s_io);
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_dbi failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "DBI IO ready");

    esp_lcd_dpi_panel_config_t dpi_cfg = {};
    dpi_cfg.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    dpi_cfg.dpi_clock_freq_mhz = LCD_DPI_CLOCK_MHZ;
    dpi_cfg.virtual_channel = 0;
    dpi_cfg.in_color_format = LCD_PIXEL_FORMAT;
    dpi_cfg.out_color_format = LCD_PIXEL_FORMAT;
    dpi_cfg.num_fbs = 2;
    dpi_cfg.flags.disable_lp = 0;
    dpi_cfg.video_timing.h_size = LCD_H_RES;
    dpi_cfg.video_timing.v_size = LCD_V_RES;
    dpi_cfg.video_timing.hsync_back_porch = 120;
    dpi_cfg.video_timing.hsync_pulse_width = 60;
    dpi_cfg.video_timing.hsync_front_porch = 106;
    dpi_cfg.video_timing.vsync_back_porch = 20;
    dpi_cfg.video_timing.vsync_pulse_width = 4;
    dpi_cfg.video_timing.vsync_front_porch = 20;

    st7703_vendor_config_t vendor_cfg = {};
    vendor_cfg.init_cmds = s_lcd_init_cmds;
    vendor_cfg.init_cmds_size = sizeof(s_lcd_init_cmds) / sizeof(s_lcd_init_cmds[0]);
    vendor_cfg.init_in_command_mode = true;
    vendor_cfg.mipi_config.dsi_bus = dsi_bus;
    vendor_cfg.mipi_config.dpi_config = &dpi_cfg;

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num = GPIO_NUM_NC;
    panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_cfg.bits_per_pixel = 24;
    panel_cfg.vendor_config = &vendor_cfg;

    s_status = "panel_create";
    err = esp_lcd_new_panel_st7703(s_io, &panel_cfg, &s_panel);
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "esp_lcd_new_panel_st7703 failed: %s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "ST7703 panel created");

    s_status = "panel_reset";
    err = esp_lcd_panel_reset(s_panel);
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "panel reset failed: %s", esp_err_to_name(err));
        return false;
    }

    s_status = "panel_init";
    err = esp_lcd_panel_init(s_panel);
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "panel init failed: %s", esp_err_to_name(err));
        return false;
    }

    s_status = "panel_disp_on";
    err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "panel display on failed: %s", esp_err_to_name(err));
        return false;
    }

    LCDSetBacklight(true);
    vTaskDelay(pdMS_TO_TICKS(50));

    ShowBootLogo();
    DrawSolidColorTest();

    /* Diagnostic: let the DSI host generate color bars for 2 seconds. */
    if (esp_lcd_dpi_panel_set_pattern(s_panel, MIPI_DSI_PATTERN_BAR_VERTICAL) == ESP_OK) {
        ESP_LOGI(TAG, "DSI test pattern ON for 2 s");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_lcd_dpi_panel_set_pattern(s_panel, MIPI_DSI_PATTERN_NONE);
        ESP_LOGI(TAG, "DSI test pattern OFF");
    }

    s_status = "lvgl_init";
    err = InitLvgl();
    if (err != ESP_OK) {
        s_last_error = err;
        ESP_LOGE(TAG, "LVGL init failed: %s", esp_err_to_name(err));
        return false;
    }

    s_status = "ready";
    ESP_LOGI(TAG, "LCD ready: %dx%d, ST7703, 2-lane DSI, RST=GPIO%d BL=GPIO%d TE=GPIO%d",
             LCD_H_RES, LCD_V_RES, LCD_PIN_RST, LCD_PIN_BL_EN, LCD_PIN_TE);
    return true;
}

esp_lcd_panel_handle_t LCDGetPanelHandle(void)
{
    return s_panel;
}

uint32_t LCDGetHRes(void)
{
    return LCD_H_RES;
}

uint32_t LCDGetVRes(void)
{
    return LCD_V_RES;
}

const char* LCDGetStatus(void)
{
    return const_cast<const char*>(s_status);
}

int LCDGetLastError(void)
{
    return s_last_error;
}



