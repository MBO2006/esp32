# ESP32 TFT ILI9341 驱动库

基于 Arduino 框架，从零手写的 ILI9341 TFT 显示屏驱动库，用于学习 SPI 协议与嵌入式驱动开发。同时包含 ESP32-S3 + ILI9341 + XPT2046 触摸屏的完整 PlatformIO 项目。

## 目录结构

```
esp32/
├── README.md
├── platformio.ini           # PlatformIO 项目配置（ESP32-S3 触摸屏项目）
├── src/
│   └── main.cpp             # 触摸屏主程序（基于 TFT_eSPI + XPT2046）
├── sketch_sep13a/
│   └── sketch_sep13a.ino    # Arduino 示例
└── libraries/
    └── TFT_ESP32/           # 手写的 ILI9341 驱动库
        ├── TFT_ESP32.h
        ├── TFT_ESP32.cpp
        └── examples/
```

### PlatformIO 触摸屏项目

`src/main.cpp` 使用 TFT_eSPI 驱动 ILI9341 显示屏，通过 bit-bang SPI 读取 XPT2046 电阻触摸芯片。显示屏和触摸使用**独立的 SPI 引脚**，避免总线冲突。

硬件接线（ESP32-S3）：

| 功能 | ESP32-S3 GPIO | ILI9341 引脚 |
|:---|:---:|:---:|
| 显示 MOSI | 16 | SDI |
| 显示 MISO | 15 | SDO |
| 显示 SCK | 18 | SCK |
| 显示 CS | 4 | CS |
| 显示 DC | 5 | DC |
| 显示 RST | 6 | RESET |
| 触摸 SDI | 21 | T_SDI |
| 触摸 SDO | 47 | T_SDO |
| 触摸 CLK | 14 | T_CLK |
| 触摸 CS | 7 | T_CS |
| 触摸 IRQ | 17 | T_IRQ |
| 电源 | 3.3V | VCC, LED |
| 地 | GND | GND |

### 手写驱动库

`libraries/TFT_ESP32/` 是从零手写的 ILI9341 驱动，用于学习 SPI 协议和嵌入式驱动开发。

## 硬件接线

ILI9341 通过 SPI 接口与 ESP32 通信，核心信号线：

| ILI9341 引脚 | ESP32 引脚 | 说明 |
|:---:|:---:|:---|
| VCC | 3.3V | 电源 |
| GND | GND | 地 |
| SCK | GPIO 18 | SPI 时钟（ESP32 硬件 SPI 默认） |
| MOSI | GPIO 23 | 主机数据输出（ESP32 硬件 SPI 默认） |
| CS | GPIO 5 | 片选，低电平有效 |
| DC | GPIO 2 | 数据/命令选择，低=命令，高=数据 |
| RST | GPIO 4 | 复位，低电平有效 |
| BLK | GPIO 15 | 背光控制 |
| MISO | - | 一般不接 |

> **注意**：具体引脚可在 `tft_spi.h` 的宏定义中修改。ESP32 的 VSPI 默认 SCK=18、MOSI=23，无需额外配置。

## 库的分层架构

```
┌─────────────────────────────────┐
│         应用层 (tft_demo.ino)    │  调用绘图 API
├─────────────────────────────────┤
│  第4层  tft_text.c              │  文字渲染、字模显示
├─────────────────────────────────┤
│  第3层  tft_draw.c              │  像素、线、矩形、圆形、填充
├─────────────────────────────────┤
│  第2层  tft_cmd.c               │  ILI9341 寄存器配置、初始化序列
├─────────────────────────────────┤
│  第1层  tft_spi.c               │  SPI 总线初始化、数据传输、引脚控制
└─────────────────────────────────┘
```

每层的学习重点：

- **第1层 SPI** → Arduino `SPI` 库、`SPI.begin()` / `SPI.transfer()`、GPIO 控制
- **第2层 CMD** → ILI9341 寄存器配置、初始化时序、数据手册阅读
- **第3层 Draw** → 像素寻址、颜色格式 RGB565、窗口设定、Bresenham 画线算法
- **第4层 Text** → 字模原理、位图渲染、字体数据结构

## 核心开发流程

### Step 1 — SPI 传输层

先让 SPI 跑通，用逻辑分析仪确认波形正确。

```c
#include <SPI.h>

// 引脚定义
#define PIN_CS   5
#define PIN_DC   2
#define PIN_RST  4
#define PIN_BLK  15

// 初始化 SPI（在 setup() 中调用）
void tft_spi_init(void)
{
    pinMode(PIN_CS,  OUTPUT);
    pinMode(PIN_DC,  OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_BLK, OUTPUT);

    SPI.begin(18, -1, 23, PIN_CS);  // SCK, MISO, MOSI, SS
    SPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_CS, HIGH);  // 默认不选中
}

// 发送命令（DC=低）
void tft_send_cmd(uint8_t cmd)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, LOW);
    SPI.transfer(cmd);
    digitalWrite(PIN_CS, HIGH);
}

// 发送数据（DC=高）
void tft_send_data(const uint8_t *data, size_t len)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    for (size_t i = 0; i < len; i++) {
        SPI.transfer(data[i]);
    }
    digitalWrite(PIN_CS, HIGH);
}

// 发送单字节数据
void tft_send_data_byte(uint8_t data)
{
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    SPI.transfer(data);
    digitalWrite(PIN_CS, HIGH);
}
```

### Step 2 — 命令与初始化

发送初始化序列，让屏幕亮起来（第一个里程碑 ✅）。

```c
void tft_init(void)
{
    // 硬件复位
    digitalWrite(PIN_RST, LOW);
    delay(10);
    digitalWrite(PIN_RST, HIGH);
    delay(150);

    // ILI9341 初始化序列（核心命令）
    tft_send_cmd(0x01);  // Software Reset
    delay(150);
    tft_send_cmd(0x28);  // Display OFF
    tft_send_cmd(0x3A);  // Pixel Format Set → 16bit (RGB565)
    tft_send_data_byte(0x55);  // 0x55 = 16bit/pixel
    tft_send_cmd(0x36);  // Memory Access Control → 设置屏幕旋转
    tft_send_data_byte(0x48);  // 横屏，RGB 顺序
    tft_send_cmd(0x29);  // Display ON
    delay(50);

    // 打开背光
    digitalWrite(PIN_BLK, HIGH);
}
```

### Step 3 — 绘图核心

理解 ILI9341 的核心机制：**先设窗口，再写像素**。

```c
// 设置绘图窗口
void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    tft_send_cmd(0x2A);  // Column Address Set
    uint8_t col_data[] = { x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF };
    tft_send_data(col_data, 4);

    tft_send_cmd(0x2B);  // Row Address Set
    uint8_t row_data[] = { y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF };
    tft_send_data(row_data, 4);

    tft_send_cmd(0x2C);  // Memory Write → 之后发的数据都是像素
}

// 画一个像素
void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    tft_set_window(x, y, x, y);
    uint8_t c[] = { color >> 8, color & 0xFF };  // RGB565 大端
    tft_send_data(c, 2);
}

// 填充矩形（最常用的高性能操作）
void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    tft_set_window(x, y, x + w - 1, y + h - 1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        SPI.transfer(hi);
        SPI.transfer(lo);
    }
    digitalWrite(PIN_CS, HIGH);
}
```

### Step 4 — 使用 SPI.write16() 加速

ESP32 Arduino 核心支持 16-bit 批量传输，比逐字节快得多：

```c
// 使用 ESP32 的 SPI.write16() 批量发送像素
void tft_fill_rect_fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    tft_set_window(x, y, x + w - 1, y + h - 1);

    uint32_t total = (uint32_t)w * h;
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, HIGH);
    for (uint32_t i = 0; i < total; i++) {
        SPI.write16(color);  // ESP32 Arduino 特有，一次发 16-bit
    }
    digitalWrite(PIN_CS, HIGH);
}
```

## 颜色格式 (RGB565)

ILI9341 默认使用 16-bit RGB565 格式：

```
Bit:  15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
      ├──────────────┼──────────────────┼──────────────┤
      │  R[4:0] (5)  │   G[5:0] (6)    │  B[4:0] (5)  │
```

常用颜色定义：

```c
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F

// RGB888 转 RGB565 宏
#define RGB565(r, g, b) (((r) >> 3) << 11 | ((g) >> 2) << 5 | ((b) >> 3))
```

## Arduino 主程序示例

```cpp
// tft_demo.ino
#include <TFT_ILI9341.h>

void setup()
{
    tft_spi_init();   // 初始化 SPI
    tft_init();       // 初始化 ILI9341

    // 全屏填充蓝色
    tft_fill_rect(0, 0, 240, 320, COLOR_BLUE);

    // 画几个彩色矩形
    tft_fill_rect(20, 20, 100, 50, COLOR_RED);
    tft_fill_rect(130, 20, 100, 50, COLOR_GREEN);
    tft_fill_rect(20, 80, 100, 50, COLOR_YELLOW);

    // 画一个白色像素
    tft_draw_pixel(120, 160, COLOR_WHITE);
}

void loop()
{
    // 主循环，可以做动画或触摸交互
}
```

## 学习路线

1. **跑通 SPI** → 用逻辑分析仪看波形，确认时序正确
2. **发初始化命令** → 屏幕亮起来 = 第一个里程碑
3. **画单个像素** → 理解 `set_window + memory_write` 核心机制
4. **写 `fill_rect`** → 性能关键函数，大量操作基于它
5. **画线（Bresenham 算法）** → 经典图形学算法
6. **添加字模** → C 数组存储字体位图，渲染文字
7. **优化性能** → 使用 `SPI.write16()` 等 ESP32 特有 API 加速

## 常用开发框架对比

| 框架 | 语言 | 优势 | 劣势 |
|------|------|------|------|
| ESP-IDF | C | 性能最优，理解底层 | 学习曲线较陡 |
| **Arduino（本项目）** | C/C++ | 生态丰富，上手简单，社区活跃 | 封装较深，但手动实现驱动可深入学习 |
| MicroPython | Python | 开发最快 | 性能最低，不适合驱动开发 |

## 开发环境

- **Arduino IDE 2.x** 或 **PlatformIO**（VSCode 插件）
- ESP32 Arduino Core（`esp32` by Espressif，通过开发板管理器安装）
- ESP32 开发板 + ILI9341 TFT 屏

### 安装 ESP32 Arduino Core

1. Arduino IDE → 文件 → 首选项 → 附加开发板管理器网址：
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. 工具 → 开发板 → 开发板管理器 → 搜索 `esp32` → 安装
3. 工具 → 开发板 → 选择 `ESP32 Dev Module`

## 参考资料

- [ILI9341 数据手册](https://www.displayfuture.com/Display/datasheet/controller/ILI9341.pdf) — 必读，命令表在 Section 8
- [ESP32 Arduino SPI 文档](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html)
- [Arduino SPI 库参考](https://www.arduino.cc/en/reference/SPI)
- [Bresenham 画线算法详解](https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm)

## 联系

GitHub: [MBO2006](https://github.com/MBO2006)
