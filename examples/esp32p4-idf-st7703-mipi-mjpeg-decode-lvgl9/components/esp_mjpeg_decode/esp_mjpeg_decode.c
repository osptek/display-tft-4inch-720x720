#include "esp_mjpeg_decode.h"
#include "esp_log.h"

static const char *TAG = "ESP_MJPEG_DECODE";

esp_err_t esp_mjpeg_decode_setup(esp_mjpeg_decode_t *mj, const char *path) {
    // 检查缓冲区大小是否有效
    if (mj->mjpeg_buffer_size == 0 || mj->output_buffer_size == 0) {
        ESP_LOGE(TAG, "缓冲区大小未设置或无效");
        return ESP_FAIL;
    }

    mj->input = fopen(path, "r");
    if (!mj->input) {
        ESP_LOGE(TAG, "无法打开文件: %s", path);
        return ESP_FAIL;
    }
    mj->read = 0;

    // 配置 JPEG 解码引擎
    jpeg_decode_engine_cfg_t decode_eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = 40,
    };
    ESP_ERROR_CHECK(jpeg_new_decoder_engine(&decode_eng_cfg, &mj->decoder_engine));

    // 分配缓冲区
    size_t tx_buffer_size, rx_buffer_size;
    jpeg_decode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };
    jpeg_decode_memory_alloc_cfg_t tx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER,
    };
    mj->mjpeg_buf = (uint8_t *)jpeg_alloc_decoder_mem(mj->mjpeg_buffer_size, &tx_mem_cfg, &tx_buffer_size);
    mj->output_buf = (uint8_t *)jpeg_alloc_decoder_mem(mj->output_buffer_size, &rx_mem_cfg, &rx_buffer_size);
    if (!mj->mjpeg_buf || !mj->output_buf) {
        ESP_LOGE(TAG, "缓冲区分配失败");
        fclose(mj->input);
        return ESP_FAIL;
    }

    return ESP_OK;
}

bool esp_mjpeg_decode_read_mjpeg_buf(esp_mjpeg_decode_t *mj) {
    if (mj->read == 0) {
        // 缓冲区为空，读取新数据
        mj->read = fread(mj->mjpeg_buf, 1, READ_BATCH_SIZE, mj->input);
    } else {
        // 将剩余数据移到缓冲区开头
        memcpy(mj->mjpeg_buf, mj->p, mj->read);
    }

    bool found_FFD8 = false;
    mj->p = mj->mjpeg_buf;
    while (mj->read > 0 && !found_FFD8) {
        while (mj->read > 1 && !found_FFD8) {
            --mj->read;
            if (*mj->p++ == 0xFF && *mj->p == 0xD8) {  // JPEG 头 (SOI)
                found_FFD8 = true;
            }
        }
        if (!found_FFD8) {
            if (*mj->p == 0xFF) {
                mj->mjpeg_buf[0] = 0xFF;
                mj->read = fread(mj->mjpeg_buf + 1, 1, READ_BATCH_SIZE, mj->input) + 1;
            } else {
                mj->read = fread(mj->mjpeg_buf, 1, READ_BATCH_SIZE, mj->input);
            }
            mj->p = mj->mjpeg_buf;
        }
    }

    if (!found_FFD8) {
        return false;
    }

    // 回退一个字节
    --mj->p;
    ++mj->read;

    // 将 JPEG 头移到缓冲区开头
    if (mj->p > mj->mjpeg_buf) {
        ESP_LOGI(TAG, "将 JPEG 头移动到开头");
        memcpy(mj->mjpeg_buf, mj->p, mj->read);
    }

    // 跳过 JPEG 头
    mj->p += 2;
    mj->read -= 2;

    if (mj->read == 0) {
        mj->read = fread(mj->p, 1, READ_BATCH_SIZE, mj->input);
    }

    bool found_FFD9 = false;
    while (mj->read > 0 && !found_FFD9) {
        while (mj->read > 1 && !found_FFD9) {
            --mj->read;
            if (*mj->p++ == 0xFF && *mj->p == 0xD9) {  // JPEG 尾 (EOI)
                found_FFD9 = true;
            }
        }
        if (!found_FFD9) {
            mj->read += fread(mj->p + mj->read, 1, READ_BATCH_SIZE, mj->input);
        }
    }

    if (found_FFD9) {
        ++mj->p;
        --mj->read;
        return true;
    }

    return false;
}

esp_err_t esp_mjpeg_decode_jpg(esp_mjpeg_decode_t *mj) {
    jpeg_decode_picture_info_t header_info;
    esp_err_t ret = jpeg_decoder_get_info(mj->mjpeg_buf, mj->p - mj->mjpeg_buf, &header_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "获取 JPEG 信息失败: %s", esp_err_to_name(ret));
        return ret;
    }
    mj->w = header_info.width;
    mj->h = header_info.height;

    uint32_t out_size;
    ret = jpeg_decoder_process(mj->decoder_engine, &mj->decode_cfg, (const uint8_t *)mj->mjpeg_buf,
                               mj->p - mj->mjpeg_buf, (uint8_t *)mj->output_buf, mj->output_buffer_size, &out_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "JPEG 解码失败: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

int16_t esp_mjpeg_decode_get_width(esp_mjpeg_decode_t *mj) {
    return mj->w;
}

int16_t esp_mjpeg_decode_get_height(esp_mjpeg_decode_t *mj) {
    return mj->h;
}

uint8_t *esp_mjpeg_decode_get_out_buf(esp_mjpeg_decode_t *mj) {
    return mj->output_buf;
}

size_t esp_mjpeg_decode_get_out_buf_size(esp_mjpeg_decode_t *mj) {
    return mj->output_buffer_size;
}

void esp_mjpeg_decode_close(esp_mjpeg_decode_t *mj) {
    if (mj->input) {
        fclose(mj->input);
        mj->input = NULL;
    }
    if (mj->mjpeg_buf) {
        free(mj->mjpeg_buf);
        mj->mjpeg_buf = NULL;
    }
    if (mj->output_buf) {
        free(mj->output_buf);
        mj->output_buf = NULL;
    }
    if (mj->decoder_engine) {
        jpeg_del_decoder_engine(mj->decoder_engine);
        mj->decoder_engine = NULL;
    }
}

esp_err_t esp_mjpeg_decode_reset(esp_mjpeg_decode_t *mj) {
    if (!mj->input) {
        ESP_LOGE(TAG, "文件句柄未打开");
        return ESP_FAIL;
    }

    // 重置文件指针到开头
    if (fseek(mj->input, 0, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "无法重置文件指针");
        return ESP_FAIL;
    }

    // 重置读取状态
    mj->read = 0;
    mj->p = mj->mjpeg_buf;

    return ESP_OK;
}