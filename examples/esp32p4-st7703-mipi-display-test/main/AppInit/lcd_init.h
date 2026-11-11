#pragma once

#include "esp_lcd_panel_ops.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool LCDInit(void);

esp_lcd_panel_handle_t LCDGetPanelHandle(void);
uint32_t LCDGetHRes(void);
uint32_t LCDGetVRes(void);
const char* LCDGetStatus(void);
int LCDGetLastError(void);
void LCDSetBacklight(bool on);

#ifdef __cplusplus
}
#endif
