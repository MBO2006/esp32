# 增修记录 (CHANGELOG)

所有重要变更都会记录在此文件。格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)。

## [v0.5.0] - 2025-09-13

### 新增
- **计时器模块** `Time.h/cpp`：秒表功能，支持开始/暂停/重置，大号字体显示时间
- **LED 控制模块** `led.h/cpp`：通过 `ledon()`/`ledoff()` 控制 GPIO 2 的 LED
- **JPEG 图片显示**：`drawJpeg()` 函数，基于 JPEGDecoder 库逐块解码渲染
- **图片转换工具** `tools/jpeg_converter.py` 和 `tools/jpeg_converter_gui.py`：支持 JPEG/PNG/BMP 转换，GUI 版带缩放、质量控制、预览功能
- 新页面 `PAGE_DIANDENG`（LED 控制页）：含"开灯"、"关灯"按钮
- 页面1 新增"计时器"和"灯"入口按钮
- `config.h` 新增 `LED_PIN 2` 定义
- `platformio.ini` 新增 `bodmer/JPEGDecoder` 依赖
- `CODE_ANALYSIS.md` 全面更新，新增计时器、LED、JPEG、开发工具等章节

### 变更
- 按钮绘制改为纯文字模式（无边框无填充，触摸区域仍有效）
- 触摸校准 Y 轴范围从 `map(rawY, 200, 1830, 0, 319)` 改为 `map(rawY, 0, 1765, 0, 319)`，修正上方按钮偏移问题
- `drawButton()` 文字垂直位置从 `btn.y + 8` 改为 `btn.y + 5`（居中）
- 中文字库从 28 字扩展到 66 字

### 修复
- 修复 `displayTouchInfo()` 函数意外删除导致链接错误
- 修复 `loop.h` 中变量定义导致的 `multiple definition` 错误（改为 extern 声明）
- 修复 `drawString` 参数过多（`100, 40`）导致编译错误
- 修复 `void AddTime()` 声明误当调用的问题
- 修复 `pinMode(LED, HIGH)` 不亮 LED 的问题（HIGH=1=INPUT，应为 OUTPUT）

---

## [v0.4.0] - 2025-09-13

### 新增
- 多页面系统：首页、页面1、页面2、功能页，支持页面间导航
- 中文字库模块 `chinese_font.h`：基于微软雅黑 24px，包含28个常用汉字
- 主循环独立模块 `loop.h/cpp`：从 main.cpp 中拆分，负责触摸检测与页面路由
- 中文按钮标签（"页面1"、"页面2"、"返回"、"功能"）
- 字体转换工具 `tools/font_converter.py` 和 `tools/font_converter_gui.py`
- 页面枚举 `Page`（PAGE_HOME / PAGE_FUNCTION / PAGE1 / PAGE2）
- `drawPageHome()`、`drawPageFunction()`、`drawpage1()`、`drawpage2()` 页面绘制函数
- `switchToHome()`、`switchToFunction()`、`gotopage1()`、`gotopage2()` 页面切换函数

### 变更
- `main.cpp` 精简为纯初始化：setup() → displayInit → drawPageHome → touchInit → loop()
- `displayTouchInfo()` 参数从4个简化为1个（仅 `bool touched`）
- 按钮定义从 main.cpp 迁移到 loop.cpp
- `CODE_ANALYSIS.md` 全面更新，新增中文字库、loop 模块、页面系统等章节
- `README.md` 目录结构更新

---

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
