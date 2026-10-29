/*
 * SPDX-FileCopyrightText: Copyright 2026 OSPTEK
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * https://github.com/osptek
 */

#include "lcd_init.h"
#include "esp_sdmmc_card.h"
#include "esp_mjpeg_decode.h"
#include "esp_log.h"

#define ROOT "/sdcard"
#define DEFAULT_MJPEG_FILENAME ROOT "/mjpeg_720_720_30fps.mjpeg"

#define FRAME_WIDTH 720  // 帧宽
#define FRAME_HEIGHT 720 // 帧高

#if BSP_LCD_COLOR_DEPTH == 16
#define OUTPUT_COLOR_BYTE 2 // 输出颜色字节
#define JPEG_DECODE_OUT_FORMAT JPEG_DECODE_OUT_FORMAT_RGB565
#elif BSP_LCD_COLOR_DEPTH == 24
#define OUTPUT_COLOR_BYTE 3 // 输出颜色字节
#define JPEG_DECODE_OUT_FORMAT JPEG_DECODE_OUT_FORMAT_RGB888
#endif

// 变量
static int total_frames = 0;
static uint64_t total_read_video = 0;
static uint64_t total_decode_video = 0;
static uint64_t total_show_video = 0;
static uint64_t start_ms, curr_ms;
static int16_t x = -1, y = -1, w = -1, h = -1;
static int lcd_width = 720, lcd_height = 720; // 假设 LCD 尺寸

// MJPEG 解码器实例
static esp_mjpeg_decode_t mjpeg = {
    .mjpeg_buffer_size = FRAME_WIDTH * FRAME_HEIGHT, // 输入缓冲区大小
    .output_buffer_size = FRAME_WIDTH * FRAME_HEIGHT * OUTPUT_COLOR_BYTE, // 输出缓冲区大小
    .decode_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT, // 输出格式
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR, // RGB 顺序
    }
};

// 播放 MJPEG 的函数
static void play_mjpeg(const char *mjpeg_filename, bool loop_playback) {
    // 初始化 MJPEG 解码器
    if (esp_mjpeg_decode_setup(&mjpeg, mjpeg_filename) != ESP_OK) {
        ESP_LOGE(TAG, "esp_mjpeg_decode_setup failed for file: %s", mjpeg_filename);
        return;
    }

    while (true) {
        while (esp_mjpeg_decode_read_mjpeg_buf(&mjpeg)) {
            // 读取时间
            curr_ms = esp_timer_get_time() / 1000;
            total_read_video += curr_ms - start_ms;
            uint64_t prev = curr_ms;

            // 解码
            if (esp_mjpeg_decode_jpg(&mjpeg) != ESP_OK) {
                ESP_LOGW(TAG, "Failed to decode JPEG frame, skipping");
                continue;
            }
            curr_ms = esp_timer_get_time() / 1000;
            total_decode_video += curr_ms - prev;
            prev = curr_ms;

            // 设置显示坐标
            if (x == -1) {
                w = esp_mjpeg_decode_get_width(&mjpeg);
                h = esp_mjpeg_decode_get_height(&mjpeg);
                x = (w > lcd_width) ? 0 : ((lcd_width - w) / 2);
                y = (h > lcd_height) ? 0 : ((lcd_height - h) / 2);
            }

            // 显示
            drawRGBBitmap(x, y, w, h, esp_mjpeg_decode_get_out_buf(&mjpeg));
            curr_ms = esp_timer_get_time() / 1000;
            total_show_video += curr_ms - prev;

            total_frames++;
        }

        // 如果不循环播放，退出循环
        if (!loop_playback) {
            break;
        }

        // 重置 MJPEG 文件读取位置
        ESP_LOGI(TAG, "Reached end of MJPEG file, restarting playback");
        if (esp_mjpeg_decode_reset(&mjpeg) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reset MJPEG decoder");
            break;
        }
    }

    // 清理解码器
    esp_mjpeg_decode_close(&mjpeg);
}

void app_main(void) {
    start_ms = esp_timer_get_time() / 1000;
    
    // 初始化 SDMMC
    esp_sdmmc_pin_config_t pin_config = {
        .clk = SDMMC_CLK,    // 时钟引脚
        .cmd = SDMMC_CMD,    // 命令引脚
        .d0 = SDMMC_D0,      // 数据0引脚
        .d1 = SDMMC_D1,      // 数据1引脚
        .d2 = SDMMC_D2,      // 数据2引脚
        .d3 = SDMMC_D3,      // 数据3引脚
        .width = SDMMC_WIDTH, // 使用宏定义的总线宽度
        .slot = SDMMC_SLOT   // 使用宏定义的卡槽编号
    };
    if (esp_sdmmc_card_init(&pin_config) != ESP_OK) {
        ESP_LOGE(TAG, "SDMMC initialization failed");
        return;
    }

    // 初始化 LCD
    if (app_lcd_init() != ESP_OK) {
        ESP_LOGE(TAG, "LCD initialization failed");
        return;
    }

    // 调用播放函数，设置文件路径和是否循环播放
    bool loop_playback = true; // 是否循环播放
    play_mjpeg(DEFAULT_MJPEG_FILENAME, loop_playback);

    // 性能报告
    uint64_t time_used = esp_timer_get_time() / 1000 - start_ms;
    float fps = time_used ? (1000.0f * total_frames / time_used) : 0.0f;
    ESP_LOGI(TAG, "ESP32-P4 MJPEG decoder");
    ESP_LOGI(TAG, "Frame size: %d x %d", esp_mjpeg_decode_get_width(&mjpeg), esp_mjpeg_decode_get_height(&mjpeg));
    ESP_LOGI(TAG, "Total frames: %d", total_frames);
    ESP_LOGI(TAG, "Time used: %llu ms", time_used);
    ESP_LOGI(TAG, "Average FPS: %.1f", fps);
    ESP_LOGI(TAG, "Read MJPEG: %llu ms (%.1f %%)", total_read_video, time_used ? (100.0f * total_read_video / time_used) : 0.0f);
    ESP_LOGI(TAG, "Decode video: %llu ms (%.1f %%)", total_decode_video, time_used ? (100.0f * total_decode_video / time_used) : 0.0f);
    ESP_LOGI(TAG, "Show video: %llu ms (%.1f %%)", total_show_video, time_used ? (100.0f * total_show_video / time_used) : 0.0f);

    // 清理 SDMMC
    esp_sdmmc_card_deinit();
    ESP_LOGI(TAG, "MJPEG end");
}
