# ESP32 TFT ILI9341 驱动库

基于 Arduino 框架，从零手写的 ILI9341 TFT 显示屏驱动库，用于学习 SPI 协议与嵌入式驱动开发。同时包含 ESP32-S3 + ILI9341 + XPT2046 触摸屏的完整 PlatformIO 项目。

## 目录结构

```
esp32/
├── platformio.ini              # PlatformIO 项目配置
├── CODE_ANALYSIS.md            # 代码深度解析文档
├── CHANGELOG.md                # 版本变更记录
├── tools/                      # 开发工具
│   ├── font_converter.py       # 字体转换脚本（命令行版）
│   ├── font_converter_gui.py   # 字体转换 GUI 工具
│   ├── jpeg_converter.py       # JPEG 图片转换脚本（命令行版）
│   └── jpeg_converter_gui.py   # JPEG 图片转换 GUI 工具
├── src/                        # 触摸屏主程序（基于 TFT_eSPI + XPT2046）
│   ├── main.cpp                # 应用入口（setup 初始化）
│   ├── config.h                # 引脚与配置定义
│   ├── display.cpp / .h        # 显示模块（页面绘制、按钮、JPEG）
│   ├── touch.cpp / .h          # 触摸驱动封装（bit-bang SPI）
│   ├── loop.cpp / .h           # 主循环（触摸检测、页面路由）
│   ├── Time.cpp / .h           # 计时器模块（秒表功能）
│   ├── led.cpp / .h            # LED 控制模块
│   ├── chinese_font.h          # 中文字库（自动生成）
│   └── img_photo.h             # JPEG 图片数据（自动生成）
├── libraries/
│   └── TFT_ESP32/              # 手写的 ILI9341 驱动库（学习用）
└── sketch_sep13a/              # Arduino 示例草图
```

## 硬件接线

### PlatformIO 触摸屏项目（ESP32-S3）

显示和触摸使用**独立 SPI 引脚**，避免总线冲突。

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

### 手写驱动库接线（ESP32）

| ILI9341 引脚 | ESP32 引脚 | 说明 |
|:---:|:---:|:---|
| VCC | 3.3V | 电源 |
| GND | GND | 地 |
| SCK | GPIO 18 | SPI 时钟 |
| MOSI | GPIO 23 | 主机数据输出 |
| CS | GPIO 5 | 片选 |
| DC | GPIO 2 | 数据/命令选择 |
| RST | GPIO 4 | 复位 |
| BLK | GPIO 15 | 背光控制 |

> **注意**：具体引脚可在源码的宏定义中修改。

---

## 环境搭建

### 方式一：PlatformIO（推荐）

1. 安装 [VSCode](https://code.visualstudio.com/)
2. 在 VSCode 扩展商店搜索 **PlatformIO IDE**，安装
3. 用 VSCode 打开本项目文件夹，PlatformIO 会自动识别 `platformio.ini` 并安装依赖

依赖库（自动安装，无需手动操作）：
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) ^2.5.43
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)

### 方式二：Arduino IDE

1. 下载安装 [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. 文件 → 首选项 → 附加开发板管理器网址，填入：
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. 工具 → 开发板 → 开发板管理器 → 搜索 `esp32` → 安装 Espressif 的包
4. 工具 → 开发板 → 选择 `ESP32S3 Dev Module`

---

## 烧录教程

### PlatformIO 方式（推荐）

#### 方法一：VSCode 图形界面

1. 用 VSCode 打开本项目文件夹（文件 → 打开文件夹 → 选择 `esp32` 目录）
2. 首次打开时，PlatformIO 会自动在底部终端安装依赖库，等待进度条完成（约 1-3 分钟）
3. 用 USB 数据线连接 ESP32-S3 开发板，确认系统识别到串口：
   - Windows：设备管理器中出现 `COMx` 端口
   - macOS/Linux：出现 `/dev/ttyUSB0` 或 `/dev/ttyACM0`
4. 点击左侧活动栏的 **PlatformIO 图标**（橙色蚂蚁头 logo）
5. 展开左侧 `PROJECT TASKS` → `esp32s3`，会看到以下操作：

   | 操作 | 说明 |
   |:---|:---|
   | **Build** | 编译项目，检查代码是否有语法错误，成功后显示 `SUCCESS` |
   | **Upload** | 编译并烧录到开发板，进度到 100% 后自动重启 |
   | **Monitor** | 打开串口监视器，查看 ESP32 输出的调试信息（波特率 115200） |
   | **Upload and Monitor** | 一键烧录 + 打开串口监控，最常用 |

6. 点击 **Upload and Monitor** 即可完成编译、烧录、监控的完整流程

#### 方法二：命令行

```bash
# 编译
pio run

# 烧录（ESP32-S3 通过 USB 直连）
pio run --target upload

# 查看串口监控（波特率 115200）
pio device monitor

# 一条命令：编译 + 烧录 + 打开串口监控
pio run --target upload && pio device monitor
```

#### 烧录失败排查

| 问题 | 解决方法 |
|:---|:---|
| 找不到串口 | 安装 [CP2102](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) 或 [CH340](http://www.wch-ic.com/downloads/CH341SER_EXE.html) 驱动 |
| ESP32-S3 无法进入下载模式 | 按住 **BOOT** 键，再按一下 **RST** 键，松开 BOOT |
| 权限不足 (Linux) | `sudo usermod -aG dialout $USER`，重新登录 |
| 端口被占用 | 关闭串口监视器或其他占用端口的程序 |

### Arduino IDE 方式

1. 工具 → 开发板 → 选择 `ESP32S3 Dev Module`
2. 工具 → 端口 → 选择对应的 COM 口
3. 点击 **上传** 按钮（→ 图标）
4. 上传完成后打开串口监视器，波特率设为 **115200**

### 手动进入烧录模式

如果自动烧录失败，手动进入下载模式：

1. **按住** BOOT 按钮（不要松手）
2. **按一下** RST 按钮后松开
3. **松开** BOOT 按钮
4. 此时 ESP32 进入下载模式，执行烧录命令
5. 烧录完成后按 RST 重启

---

## 手写驱动库架构

`libraries/TFT_ESP32/` 是从零手写的 ILI9341 驱动，用于学习：

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
│  第1层  tft_spi.c               │  SPI 总线初始化、数据传输
└─────────────────────────────────┘
```

| 层级 | 学习重点 |
|:---|:---|
| 第1层 SPI | Arduino SPI 库、`SPI.begin()` / `SPI.transfer()` |
| 第2层 CMD | ILI9341 寄存器配置、初始化时序、数据手册阅读 |
| 第3层 Draw | 像素寻址、RGB565 颜色格式、Bresenham 画线算法 |
| 第4层 Text | 字模原理、位图渲染、字体数据结构 |

## 学习路线

1. **跑通 SPI** → 用逻辑分析仪看波形，确认时序正确
2. **发初始化命令** → 屏幕亮起来 = 第一个里程碑
3. **画单个像素** → 理解 `set_window + memory_write` 核心机制
4. **写 `fill_rect`** → 性能关键函数，大量操作基于它
5. **画线（Bresenham 算法）** → 经典图形学算法
6. **添加字模** → C 数组存储字体位图，渲染文字
7. **优化性能** → 使用 ESP32 特有 API 加速传输

## 参考资料

- [ILI9341 数据手册](https://www.displayfuture.com/Display/datasheet/controller/ILI9341.pdf) — 命令表在 Section 8
- [ESP32 Arduino SPI 文档](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html)
- [TFT_eSPI 库文档](https://github.com/Bodmer/TFT_eSPI)
- [PlatformIO ESP32 文档](https://docs.platformio.org/en/latest/boards/espressif32/)

## 常用 Git 命令

```bash
git add .                    # 暂存所有修改
git commit -m "描述"          # 提交
git push origin main         # 推送到 GitHub
git pull origin main         # 拉取最新代码
git status                   # 查看状态
git log --oneline            # 查看提交历史
```

## 联系

GitHub: [MBO2006](https://github.com/MBO2006)
