# 增修记录 (CHANGELOG)

所有重要变更都会记录在此文件。格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)。

## [v0.3.0] - 2025-09-13

### 新增
- 代码模块化：拆分为 `config.h`、`touch.h/cpp`、`display.h/cpp`、`main.cpp`
- 新增 `CODE_ANALYSIS.md` 代码解析文档

### 变更
- `main.cpp` 精简为应用逻辑，不再包含底层驱动代码
- 触摸驱动和显示驱动独立为模块

---

## [v0.2.0] - 2025-09-13

### 新增
- XPT2046 电阻触摸驱动（bit-bang SPI）
- 显示屏与触摸使用独立 SPI 引脚，避免总线冲突
- 触摸原始值显示功能
- TFT_eSPI 字体支持（LOAD_GLCD、LOAD_FONT2、LOAD_FONT4）

### 变更
- MISO 引脚从 GPIO 9 改为 GPIO 15
- 触摸 SPI 引脚独立：T_SDI=21, T_SDO=47, T_CLK=14, T_CS=7
- 添加 `USE_HSPI_PORT` 使用 SPI3 驱动显示屏
- 添加 `ARDUINO_USB_CDC_ON_BOOT=1` 启用 USB 串口输出

### 修复
- 修复 GPIO 11/12/13 与 SPI Flash 冲突导致黑屏
- 修复 `USER_SETUP_LOADED` 导致字体未编译，文字不显示
- 修复触摸与显示共享 SPI 总线时 MISO 数据冲突
- 修复 T_SDI 与 SDI 模块内部不连通，触摸芯片收不到命令

---

## [v0.1.0] - 2025-09-13

### 新增
- 基本 ESP32-S3 + ILI9341 显示项目初始化
- TFT_eSPI 库集成
- LED 闪烁测试程序
- PlatformIO 项目配置

### 引脚分配（初始版本，已废弃）
- MOSI=11, SCK=12, CS=10, DC=8, RST=9, MISO=13
- 问题：GPIO 11/12/13 是 SPI Flash 引脚，无法使用

---

## 引脚变更历史

| 版本 | MOSI | SCK | MISO | CS | DC | RST | T_CS | T_IRQ | T_SDI | T_SDO | T_CLK |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| v0.1.0 | 11 | 12 | 13 | 10 | 8 | 9 | - | - | - | - | - |
| v0.2.0 | 16 | 18 | 15 | 4 | 5 | 6 | 7 | 17 | 21 | 47 | 14 |
