#include "DisplayTest.h"

#include <cstdio>

#include "esp_log.h"
#include "esp_timer.h"
#include "lcd_init.h"
#include "lvgl.h"

namespace {

constexpr char kTag[] = "DisplayTest";
constexpr int32_t kScreenW = 720;
constexpr int32_t kScreenH = 720;

const uint32_t kBarColors[] = {
    0xff0000, 0x00ff00, 0x0000ff, 0xffff00,
    0xff00ff, 0x00ffff, 0xffffff, 0x000000,
};
const char* kBarNames[] = {
    "RED", "GREEN", "BLUE", "YELLOW",
    "MAGENTA", "CYAN", "WHITE", "BLACK",
};

lv_obj_t* s_ball;
lv_obj_t* s_uptime;
uint32_t s_tick;

void AddInfoRow(lv_obj_t* parent, const char* key, const char* value)
{
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), 28);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* k = lv_label_create(row);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_font(k, &lv_font_montserrat_14, 0);

    lv_obj_t* v = lv_label_create(row);
    lv_label_set_text(v, value);
    lv_obj_set_style_text_color(v, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_text_font(v, &lv_font_montserrat_16, 0);
}

void CreateHeader(lv_obj_t* screen)
{
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, lv_pct(100), 92);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(header, 24, 0);
    lv_obj_set_style_pad_right(header, 24, 0);
    lv_obj_set_style_pad_top(header, 14, 0);
    lv_obj_set_style_pad_bottom(header, 10, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(header, 2, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "ESP32-P4 + YDP400B001-V1");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    lv_obj_t* subtitle = lv_label_create(header);
    lv_label_set_text(subtitle, "720 x 720 IPS  |  ST7703  |  2-lane MIPI-DSI");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
}

void CreateInfoPanel(lv_obj_t* screen)
{
    lv_obj_t* panel = lv_obj_create(screen);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, lv_pct(100), 170);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x0b1220), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(panel, 24, 0);
    lv_obj_set_style_pad_right(panel, 24, 0);
    lv_obj_set_style_pad_top(panel, 8, 0);
    lv_obj_set_style_pad_bottom(panel, 8, 0);
    lv_obj_set_style_pad_row(panel, 2, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    AddInfoRow(panel, "Panel", "YDP400B001-V1  4.0 inch  720x720");
    AddInfoRow(panel, "Driver IC", "ST7703  (RGB888)");
    AddInfoRow(panel, "DSI", "2 lanes, 600 Mbps/lane, 46 MHz DPI");
    AddInfoRow(panel, "Backlight", "SY7200 boost, EN = GPIO20");
    AddInfoRow(panel, "Touch", "not populated on this panel");
}

void CreateColorBars(lv_obj_t* screen)
{
    lv_obj_t* bars = lv_obj_create(screen);
    lv_obj_remove_style_all(bars);
    lv_obj_set_size(bars, lv_pct(100), 230);
    lv_obj_set_style_bg_color(bars, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(bars, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(bars, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(bars, 0, 0);
    lv_obj_set_style_pad_column(bars, 0, 0);
    lv_obj_set_style_pad_row(bars, 0, 0);
    lv_obj_set_style_border_width(bars, 0, 0);
    lv_obj_set_style_radius(bars, 0, 0);
    lv_obj_clear_flag(bars, LV_OBJ_FLAG_SCROLLABLE);

    for (size_t i = 0; i < sizeof(kBarColors) / sizeof(kBarColors[0]); ++i) {
        lv_obj_t* bar = lv_obj_create(bars);
        lv_obj_remove_style_all(bar);
        lv_obj_set_height(bar, lv_pct(100));
        lv_obj_set_flex_grow(bar, 1);
        lv_obj_set_style_bg_color(bar, lv_color_hex(kBarColors[i]), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(bar, 0, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 0, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* label = lv_label_create(bar);
        lv_label_set_text(label, kBarNames[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        const bool dark_text = (i == 1 || i == 3 || i == 5 || i == 6);
        lv_obj_set_style_text_color(label, lv_color_hex(dark_text ? 0x111111 : 0xffffff), 0);
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -8);
    }
}

void CreateGrayscale(lv_obj_t* screen)
{
    lv_obj_t* gray = lv_obj_create(screen);
    lv_obj_remove_style_all(gray);
    lv_obj_set_size(gray, lv_pct(100), 64);
    lv_obj_set_flex_flow(gray, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(gray, 0, 0);
    lv_obj_set_style_pad_row(gray, 0, 0);
    lv_obj_set_style_pad_column(gray, 0, 0);
    lv_obj_set_style_border_width(gray, 0, 0);
    lv_obj_set_style_radius(gray, 0, 0);
    lv_obj_clear_flag(gray, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 16; ++i) {
        const uint8_t v = static_cast<uint8_t>(i * 17);
        lv_obj_t* block = lv_obj_create(gray);
        lv_obj_remove_style_all(block);
        lv_obj_set_height(block, lv_pct(100));
        lv_obj_set_flex_grow(block, 1);
        lv_obj_set_style_bg_color(block, lv_color_make(v, v, v), 0);
        lv_obj_set_style_bg_opa(block, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(block, 0, 0);
        lv_obj_set_style_border_width(block, 0, 0);
        lv_obj_set_style_radius(block, 0, 0);
        lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    }
}

void CreateFooter(lv_obj_t* screen)
{
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_remove_style_all(footer);
    lv_obj_set_width(footer, lv_pct(100));
    lv_obj_set_flex_grow(footer, 1);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x0e1116), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(footer, 24, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* msg = lv_label_create(footer);
    lv_label_set_text(msg, "Display bring-up OK\nColor bars, grayscale ramp and refresh animation");
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(msg, lv_color_hex(0x22c55e), 0);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_16, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 2);

    s_ball = lv_obj_create(footer);
    lv_obj_remove_style_all(s_ball);
    lv_obj_set_size(s_ball, 22, 22);
    lv_obj_set_style_radius(s_ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ball, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_bg_opa(s_ball, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(s_ball, 14, 0);
    lv_obj_set_style_shadow_color(s_ball, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_style_shadow_opa(s_ball, LV_OPA_60, 0);
    lv_obj_align(s_ball, LV_ALIGN_BOTTOM_LEFT, 0, -8);

    s_uptime = lv_label_create(footer);
    lv_label_set_text(s_uptime, "uptime 0 s   tick 0");
    lv_obj_set_style_text_color(s_uptime, lv_color_hex(0x64748b), 0);
    lv_obj_set_style_text_font(s_uptime, &lv_font_montserrat_12, 0);
    lv_obj_align(s_uptime, LV_ALIGN_BOTTOM_LEFT, 0, -38);
}

void TimerCb(lv_timer_t* timer)
{
    (void)timer;
    ++s_tick;

    const int32_t travel = kScreenW - 48 - 22;
    const int32_t x = static_cast<int32_t>((s_tick * 9U) % static_cast<uint32_t>(travel));
    lv_obj_set_x(s_ball, x);

    const int64_t uptime_s = esp_timer_get_time() / 1000000;
    lv_label_set_text_fmt(s_uptime, "uptime %lld s   tick %lu", uptime_s,
                          static_cast<unsigned long>(s_tick));
}

}  // namespace

extern "C" void DisplayTest_Create(void)
{
    lv_obj_t* screen = lv_screen_active();
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, kScreenW, kScreenH);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0e1116), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_pad_row(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    CreateHeader(screen);
    CreateInfoPanel(screen);
    CreateColorBars(screen);
    CreateGrayscale(screen);
    CreateFooter(screen);

    lv_timer_create(TimerCb, 33, nullptr);
    ESP_LOGI(kTag, "Display test UI created");
}
