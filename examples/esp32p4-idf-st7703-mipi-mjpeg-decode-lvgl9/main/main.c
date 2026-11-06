/*
 * SPDX-FileCopyrightText: Copyright 2026 OSPTEK
 * SPDX-License-Identifier: CC-BY-4.0
 *
 * https://github.com/osptek
 */

#include "lvgl_init.h"
#include "esp_sdmmc_card.h"
#include "esp_mjpeg_decode.h"

#define ROOT "/sdcard"
#define MJPEG_FILENAME ROOT "/mjpeg_720_720_30fps.mjpeg"

#define FRAME_WIDTH 720 // 帧宽
#define FRAME_HEIGHT 720 // 帧高

#if LV_COLOR_DEPTH == 16
#define JPEG_DECODE_OUT_FORMAT JPEG_DECODE_OUT_FORMAT_RGB565
#elif LV_COLOR_DEPTH == 24
#define JPEG_DECODE_OUT_FORMAT JPEG_DECODE_OUT_FORMAT_RGB888
#endif

// MJPEG 解码器实例
static esp_mjpeg_decode_t mjpeg = {
    .mjpeg_buffer_size = FRAME_WIDTH * FRAME_HEIGHT, // 输入缓冲区大小
    .output_buffer_size = FRAME_WIDTH * FRAME_HEIGHT * (LV_COLOR_DEPTH / 8),    // 输出缓冲区大小
    .decode_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT, // 输出格式
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR, // RGB 顺序
    }
};

// 变量
static lv_obj_t *video_img = NULL;        // LVGL 图像对象
static lv_image_dsc_t img_dsc;            // 图像描述符（用于 raw 缓冲区）

// MJPEG 播放函数
static void play_mjpeg(esp_mjpeg_decode_t *mjpeg, lv_obj_t *img_obj, lv_image_dsc_t *img_dsc, bool loop_playback) {
    while (true) {
        // 读取 MJPEG 文件直到结束
        while (esp_mjpeg_decode_read_mjpeg_buf(mjpeg)) {
            // 解码到输出缓冲区
            if (esp_mjpeg_decode_jpg(mjpeg) != ESP_OK) continue;  // 解码失败则跳过

            // LVGL 线程安全锁
            lvgl_port_lock(0);

            // 更新 LVGL 图像源（使用解码缓冲区）
            img_dsc->header.w = esp_mjpeg_decode_get_width(mjpeg);
            img_dsc->header.h = esp_mjpeg_decode_get_height(mjpeg);
            img_dsc->data_size = img_dsc->header.w * img_dsc->header.h * (LV_COLOR_DEPTH / 8);
            img_dsc->data = (uint8_t *)mjpeg->output_buf;  // 指向解码器输出缓冲区
            lv_image_set_src(img_obj, img_dsc);

            // 刷新 LVGL 显示（触发 flush）
            lv_refr_now(lvgl_disp);  // 立即刷新（适合视频）

            // 释放 LVGL 锁
            lvgl_port_unlock();
        }

        // 检查是否需要循环播放
        if (!loop_playback) {
            break; // 如果不循环播放，退出外层循环
        }

        // 重置 MJPEG 文件读取位置
        ESP_LOGI(TAG, "Reached end of MJPEG file, restarting playback");
        if (esp_mjpeg_decode_reset(mjpeg) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to reset MJPEG decoder");
            break;
        }
    }
}

void app_main(void) {

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
    esp_sdmmc_card_init(&pin_config);

    // 初始化 LCD
    ESP_ERROR_CHECK(app_lcd_init());

    // 初始化 LVGL
    ESP_ERROR_CHECK(app_lvgl_init());

    // 初始化 MJPEG 解码器
    if (esp_mjpeg_decode_setup(&mjpeg, MJPEG_FILENAME) != ESP_OK) {
        ESP_LOGE(TAG, "esp_mjpeg_decode_setup 失败");
        esp_sdmmc_card_deinit();
        return;
    }

    // 创建 LVGL 屏幕和图像对象（只创建一次）
    lv_obj_t *screen = lv_screen_active();
    video_img = lv_image_create(screen);
    lv_obj_center(video_img);  // 居中显示
    lv_obj_set_size(video_img, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);  // 设置为全屏

    // 初始化图像描述符（header 固定，data 在循环中更新）
    memset(&img_dsc, 0, sizeof(img_dsc));
    img_dsc.header.cf = LV_COLOR_FORMAT;  // 颜色格式

    bool loop_playback = true; // 是否循环播放
    // 播放 MJPEG
    play_mjpeg(&mjpeg, video_img, &img_dsc, loop_playback);

    // 清理
    esp_mjpeg_decode_close(&mjpeg);

    // 删除 LVGL 图像对象
    if (video_img) {
        lv_obj_delete(video_img);
        video_img = NULL;
    }

    esp_sdmmc_card_deinit();
    ESP_LOGI(TAG, "MJPEG 结束");
}