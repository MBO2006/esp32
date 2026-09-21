# ESP32-S3 智能手表 — LVGL + ILI9341 + XPT2046

基于 ESP32-S3 + ILI9341 TFT (240×320) + XPT2046 触摸屏的 LVGL v9 智能手表项目。

## 目录结构

```
esp32/
├── platformio.ini              # PlatformIO 配置（库依赖、编译选项）
├── lv_conf.h                   # LVGL 配置文件（色深、字体、控件）
├── src/
│   ├── main.cpp                # 应用入口
│   ├── config.h                # 引脚定义
│   ├── lvgl_setup.cpp / .h     # LVGL 显示/触摸驱动
│   ├── display.cpp / .h        # TFT_eSPI 底层驱动
│   ├── touch.cpp / .h          # XPT2046 触摸驱动（bit-bang SPI）
│   ├── Timer.cpp / .h          # 计时器逻辑
│   ├── loop.cpp / .h           # LVGL 回调注册 + 定时器更新
│   ├── led.cpp / .h            # LED 控制
│   ├── openmv.cpp / .h         # OpenMV 摄像头（SPI 通信）
│   └── screens/
│       ├── ui_screens.cpp / .h # 所有界面（表盘/计时器/LED）
│       └── ...
├── picture/                    # 开发过程截图
└── tools/                      # 开发工具
```

## 硬件接线

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

---

## 踩坑记录与解决方案

### 坑1：`time.h` 头文件冲突 — "template with C linkage"

**现象：**
编译时报大量 `template with C linkage` 错误，涉及 `<functional>`、`<stat.h>` 等系统头文件。

**原因：**
`platformio.ini` 的 `build_flags` 中加了 `-Isrc`，把项目 `src/` 目录加入了**全局**编译路径。Windows 文件系统不区分大小写，当系统头文件 `sys/stat.h` 里 `#include <time.h>` 时，编译器在 `src/` 中找到了我们的 `Timer.h`，误当作系统 `<time.h>` 引入。系统头文件被包裹在 `extern "C"` 块中，而我们的头文件引用了 C++ 的 `Arduino.h`，导致 C++ 模板出现在 C 链接块里。

**解决：**
从 `build_flags` 中删除 `-Isrc`。PlatformIO 的 Arduino 框架会自动把 `src/` 加到 include 路径，不需要手动指定。

```diff
 build_flags =
-    -Isrc
     -Isrc/screens
```

**教训：** 永远不要用 `-I` 把项目源码目录加到全局编译路径，会污染系统头文件搜索。用 `include/` 目录或让框架自动处理。

---

### 坑2：`lv_conf.h` 找不到 — "Possible failure to include lv_conf.h"

**现象：**
LVGL 编译时报 `#pragma message: Possible failure to include lv_conf.h`。

**原因：**
LVGL 在多个相对路径搜索 `lv_conf.h`，但 `-Isrc` 已被删除，编译器找不到它。

**解决：**
1. 把 `lv_conf.h` 放到**项目根目录**（与 `platformio.ini` 同级）
2. 用 `LV_CONF_PATH` 指定路径（**必须带引号**）：

```diff
-    -D LV_CONF_INCLUDE_SIMPLE
+    -D LV_CONF_PATH=\"lv_conf.h\"
```

**教训：** LVGL 的 `LV_CONF_PATH` 宏需要转义引号 `\"...\"`，否则 `#include` 指令报语法错误。

---

### 坑3：OpenMV 与 TFT_eSPI 的 SPI 总线冲突

**现象：**
烧录后串口无输出，屏幕无显示。

**原因：**
TFT_eSPI 使用 HSPI (SPI2) 驱动屏幕，OpenMV 的 `openmvInit()` 也初始化了 HSPI（用不同引脚），重写了 SPI2 的配置，导致 TFT_eSPI 的 SPI 通信完全失效。

**解决：**
暂时注释掉 `openmvInit()`，后续需要将 OpenMV 迁移到 SPI3 并分配独立引脚。

**教训：** ESP32-S3 上 HSPI = SPI2，多个模块共用同一个 SPI 外设会互相覆盖配置。要用独立 SPI 外设（SPI2 / SPI3）或用 GPIO 模拟。

---

### 坑4：PSRAM 不可用 — 缓冲区分配失败

**现象：**
串口显示 `[FATAL] No memory for display buffer!`。

**原因：**
PlatformIO 的板子定义 `esp32-s3-devkitc-1` 对应的是 **N8**（8MB Flash，**无 PSRAM**），不是 N16R8。150KB 的全帧缓冲区（240×320×2字节）分配两个需要300KB，内部 SRAM 放不下两个。

**解决：**
改用**单缓冲 + PARTIAL 渲染模式**，只分配 40 行的缓冲区（约 19KB）：

```c
size_t buf_bytes = 240 * 40 * sizeof(lv_color_t);  // ~19KB
draw_buf = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL);
lv_display_set_buffers(disp, draw_buf, NULL, buf_bytes,
                       LV_DISPLAY_RENDER_MODE_PARTIAL);
```

**教训：** 没有 PSRAM 的 ESP32-S3 内部 SRAM 只有约200KB 可用。LVGL 单缓冲 PARTIAL 模式是最省内存的方案。

---

### 坑5：LVGL `.c` 文件包含 C++ 头文件

**现象：**
编译 `lvgl_setup.c` 报错：`class Print;` — C 语言不认识 `class`。

**原因：**
`lvgl_setup.c` 文件扩展名是 `.c`（C 编译器），但 `#include <TFT_eSPI.h>` 是 C++ 头文件。

**解决：**
把文件重命名为 `lvgl_setup.cpp`，让编译器用 C++ 模式编译。

**教训：** 只要 `#include` 了任何 Arduino / TFT_eSPI / LVGL 的 C++ 头文件，文件扩展名必须是 `.cpp`。

---

### 坑6：屏幕显示红色变蓝色 — RGB 颜色通道反转

**现象：**
LVGL 能显示了，但 `lv_color_hex(0xFF0000)`（红色）显示为蓝色。

**原因：**
ILI9341 面板有两种：RGB 排列和 BGR 排列。我们的面板是 BGR 排列，LVGL 输出的 RGB565 数据高低字节顺序与面板不匹配。

**解决：**
在 flush 回调中手动交换每个像素的高低字节：

```c
uint16_t *p = (uint16_t *)px_map;
uint32_t total = w * h;
for (uint32_t i = 0; i < total; i++) {
    p[i] = (p[i] >> 8) | (p[i] << 8);
}
```

**教训：** LVGL 的 `LV_COLOR_16_SWAP` 和 `LV_COLOR_FORMAT_RGB565_SWAP` 在某些版本/配置下可能不起作用。手动字节交换是最可靠的方案。

---

### 坑7：`millis()` 函数指针类型不匹配

**现象：**
`lv_tick_set_cb(millis)` 报错：`invalid conversion from 'unsigned long (*)()' to 'uint32_t (*)()'`

**原因：**
Arduino 的 `millis()` 返回 `unsigned long`（ESP32 上是32位），但 LVGL 要求返回 `uint32_t`。虽然实际大小相同，但 C++ 类型系统不允许隐式转换。

**解决：**
写一个包装函数：

```c
static uint32_t my_millis(void) { return (uint32_t)millis(); }
lv_tick_set_cb(my_millis);
```

---

## LVGL 集成架构

```
setup():
  displayInit()          → TFT_eSPI SPI 初始化
  touchInit()            → XPT2046 bit-bang SPI 初始化
  lvgl_setup_init()      → LVGL 内核 + 显示驱动 + 触摸驱动
  screens_init()         → 创建表盘/计时器/LED 三个界面
  app_init_callbacks()   → 注册 LED/计时器事件回调

loop():
  lvgl_setup_loop()      → lv_timer_handler() 驱动渲染
```

---

## 快速开始

1. 安装 [PlatformIO](https://platformio.org/)
2. 打开本项目文件夹
3. 修改 `platformio.ini` 中的引脚配置（如需要）
4. 编译：`pio run`
5. 烧录：`pio run --target upload`
6. 串口监控：`pio device monitor`

依赖库（PlatformIO 自动安装）：
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) ^2.5.43
- [LVGL](https://github.com/lvgl/lvgl) ^9.2.0
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)

## 联系

GitHub: [MBO2006](https://github.com/MBO2006)
