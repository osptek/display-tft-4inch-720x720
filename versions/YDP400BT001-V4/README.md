<p align="left"><img alt="OSPTEK" src="../../images/logo.png" width="200" /></p>

<h1 align="center">OSPTEK 4″ TFT 显示屏 720×720（YDP400BT001-V4）</h1>

<p align="center"><b>TFT 模组 · MIPI/I2C · ST7703I</b></p>

<p align="center"><a href="./README_EN.md">English</a> | 简体中文 · <a href="../../README.md">规格族索引</a></p>

<p align="center">
  <img alt="Interface: MIPI/I2C" src="https://img.shields.io/badge/Interface-MIPI%20%C2%B7%20I2C-27AE60?style=flat-square" />
  <img alt="Driver: ST7703I" src="https://img.shields.io/badge/Driver-ST7703I-E7352C?style=flat-square" />
  <img alt="Touch: GT911" src="https://img.shields.io/badge/Touch-GT911-F39C12?style=flat-square" />
</p>

<p align="center"><img alt="OSPTEK 4 寸 720×720 TFT 模组宣传图" src="./images/product.png" width="640" /></p>

## 目录

- [规格参数](#规格参数)
- [相关资料](#相关资料)
- [仓库结构](#仓库结构)
- [购买链接](#购买链接)
- [技术支持](#技术支持)

---

## 规格参数

| 项目 | 规格 |
| ---- | ---- |
| 尺寸 | 4 英寸 |
| 类型 | TFT |
| 分辨率 | 720×720 |
| 外形尺寸 | 74.33 x 78.38 mm |
| 有效显示区域 | 71.93 x 71.93 mm |
| 接口 | MIPI/I2C |
| 驱动 IC | ST7703I |
| 触摸驱动 | GT911 |

> 完整外形尺寸、FPC 定义、供电与时序以产品规格书 / 驱动手册为准。

## 相关资料

| 分类 | 文件 |
| ---- | ---- |
| 规格文档 | [`GT_911_Datasheet_20140718_Rev_08_daa11566d3.pdf`](./datasheets/GT_911_Datasheet_20140718_Rev_08_daa11566d3.pdf) |
| 规格文档 | [`ST7703DA-H3_DS_V0.01_20200819.pdf`](./datasheets/ST7703DA-H3_DS_V0.01_20200819.pdf) |
| 规格文档 | [`YDP_400_BT_001_V4_058148418c.pdf`](./datasheets/YDP_400_BT_001_V4_058148418c.pdf) |
| 转接板资料 | [`2.1&4“转接板SCRREN_P4_yuying.pdf`](./adapter/2.1%264%E2%80%9C%E8%BD%AC%E6%8E%A5%E6%9D%BFSCRREN_P4_yuying.pdf) |

## 仓库结构

```text
display-tft-4inch-720x720/ # 仓库根（导航见 ../../README.md，示例工程见根目录 examples/）
└── versions/
    └── YDP400BT001-V4/        # 本料号完整资料
        ├── README.md
        ├── README_EN.md
        ├── osptek.yml
        ├── images/
        ├── datasheets/       # 产品规格书 + 驱动/触摸 IC 数据手册
        └── adapter/          # 转接板资料 + 转接板图片
```

## 购买链接

见[仓库首页](../../README.md#购买链接)。

## 技术支持

见[仓库首页](../../README.md#技术支持)。

---

<p align="center"><sub>© 2026 OSPTEK 鱼鹰光电 · 本仓库资料采用 CC BY 4.0 许可</sub></p>
