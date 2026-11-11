# 示例工程 / Sample Projects

本目录收录本规格屏幕的示例工程，每个子文件夹是一个独立工程，打开对应文件夹按其中的 README 编译即可。
这些工程是参考代码，引脚、背光、触摸等配置请按你的硬件修改；选用前请对照仓库首页各型号的驱动 IC 和接口确认。

This folder holds sample projects for this display spec. Each sub-folder is a standalone project — open it and follow its own README to build.
They are reference code; adjust pins, backlight, touch and other settings to your hardware. Before choosing one, check the driver IC and interface against the models listed on the repository home page.

| 例程 / Project | 平台 / Platform | 驱动 IC / Driver IC | 接口 / Interface | 说明 / Description |
|---|---|---|---|---|
| [`esp32p4-st7703-mipi-display-test`](./esp32p4-st7703-mipi-display-test/) | ESP32-P4 · ESP-IDF | ST7703 | MIPI | 显示测试、LVGL 彩条/灰阶/刷新动画、TE 防撕裂 / display test, LVGL color bars/gray scale/refresh animation, TE tear-free |

如有问题请联系技术支持。 / Contact technical support if you have questions.
