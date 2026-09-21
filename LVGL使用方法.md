# LVGL 使用方法 — 基于本项目框架

> 硬件：ESP32-S3 + ILI9341 TFT (240×320) + XPT2046 触摸屏
> 框架：LVGL v9.2 + Arduino + PlatformIO

---

## 一、项目文件结构

```
src/
├── main.cpp              ← 入口，setup() + loop()
├── config.h              ← 引脚定义、常量
├── display.h / display.cpp  ← TFT 底层绘制（drawJpeg 等）
├── touch.h / touch.cpp     ← XPT2046 触摸底层读取
├── lvgl_setup.h / .cpp     ← LVGL 初始化、flush、tick、触摸驱动
├── led.h / led.cpp         ← LED 控制
├── Timer.h / Timer.cpp     ← 计时器逻辑
├── loop.cpp                ← 回调注册 + 业务逻辑
└── screens/
    ├── ui_screens.h         ← 屏幕枚举 + 公开接口
    └── ui_screens.cpp       ← 所有界面创建代码
```

**调用链：**

```
main.cpp setup()
  ├── displayInit()          // TFT 初始化
  ├── touchInit()            // 触摸初始化
  ├── lvgl_setup_init()      // LVGL 内核 + 显示驱动 + 触摸驱动
  ├── screens_init()         // 创建所有界面，默认显示表盘
  └── app_init_callbacks()   // 注册业务回调（计时器更新、LED）

main.cpp loop()
  └── lvgl_setup_loop()      // lv_timer_handler() 驱动渲染
```

---

## 二、核心文件详解

### 2.1 `lvgl_setup.cpp` — LVGL 引擎

这是 LVGL 与硬件的桥梁，包含三个关键回调：

#### 显示回调 (flush_cb)

```cpp
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // 1. 交换高低字节（ILI9341 需要 RGB565 大端）
    // 2. 通过 TFT_eSPI 推送像素
    // 3. 通知 LVGL 刷新完成
    lv_display_flush_ready(disp);
}
```

#### 触摸回调 (read_cb)

```cpp
static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    // 读取 XPT2046 原始坐标 → map 到屏幕坐标
    // z > 阈值 → 按下，否则释放
}
```

#### Tick 回调

```cpp
static void tick_cb(lv_timer_t *) { lv_tick_inc(5); }  // 每 5ms 递增
```

### 2.2 `screens/ui_screens.cpp` — 界面创建

所有界面都在这里用代码构建，**没有使用 LVGL SquareLine Studio**。

**已创建的界面：**

| 屏幕 ID | 界面 | 说明 |
|---------|------|------|
| `SCREEN_WATCHFACE` | 表盘 | 时间 + 日期 + 状态栏 + 导航按钮 |
| `SCREEN_TIMER` | 计时器 | 时间显示 + 开始/暂停/重置按钮 |
| `SCREEN_LED` | LED 控制 | 灯泡图标 + 开关 + 状态文字 |

### 2.3 `loop.cpp` — 业务逻辑

UI 和业务逻辑的连接点：

```cpp
void app_init_callbacks(void)
{
    // 绑定 LED 回调
    ui_led_set_callbacks(ledon, ledoff, NULL);

    // 创建 LVGL 定时器，每秒更新计时器显示
    lv_timer_create(timer_update_cb, 1000, NULL);
}
```

---

## 三、如何添加一个新界面

以添加"亮度调节"界面为例，**共 3 步**：

### 步骤 1：在 `ui_screens.h` 添加枚举

```cpp
typedef enum {
    SCREEN_WATCHFACE,
    SCREEN_TIMER,
    SCREEN_LED,
    SCREEN_BRIGHTNESS,   // ← 新增
    SCREEN_COUNT
} screen_id_t;
```

添加公开函数（如需更新数据）：

```cpp
void screens_update_brightness(int percent);
```

### 步骤 2：在 `ui_screens.cpp` 创建界面

```cpp
/* 新增静态控件指针 */
static lv_obj_t *br_label_value = NULL;

/* 导航回调 */
static void nav_brightness(lv_event_t *e) {
    (void)e;
    screens_nav(SCREEN_BRIGHTNESS);
}

/* 返回表盘 */
static void nav_back_from_brightness(lv_event_t *e) {
    (void)e;
    screens_nav(SCREEN_WATCHFACE);
}

/* 界面创建函数 */
static void create_brightness(void)
{
    lv_obj_t *scr = create_base(COLOR_BG);
    create_header(scr, "亮度调节", nav_back_from_brightness);

    /* 内容区 */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_W, 250);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(content, 20, 0);

    /* 百分比文字 */
    br_label_value = lv_label_create(content);
    lv_label_set_text(br_label_value, "50%");
    lv_obj_set_style_text_color(br_label_value, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(br_label_value, &lv_font_montserrat_44, 0);

    /* 滑块 */
    lv_obj_t *slider = lv_slider_create(content);
    lv_obj_set_width(slider, 200);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, on_slider_change, LV_EVENT_VALUE_CHANGED, NULL);

    screens[SCREEN_BRIGHTNESS] = scr;
}

/* 注册到 screens_init() */
void screens_init(void)
{
    create_watchface();
    create_timer();
    create_led();
    create_brightness();   // ← 新增

    lv_screen_load(screens[SCREEN_WATCHFACE]);
}
```

### 步骤 3：在表盘添加入口按钮

```cpp
// 在 create_watchface() 的 row 中加一个按钮
create_btn(row, LV_SYMBOL_IMAGE " 亮度", COLOR_BTN_BG, nav_brightness);
```

**完成！** 运行后表盘多了一个"亮度"按钮，点击进入新界面。

---

## 四、常用 LVGL 控件速查

### 标签 (Label)

```cpp
lv_obj_t *lbl = lv_label_create(parent);
lv_label_set_text(lbl, "Hello");           // 设置文字
lv_label_set_text_fmt(lbl, "%d%%", 50);    // 格式化
lv_obj_set_style_text_color(lbl, lv_color_hex(0xFF0000), 0);  // 红色
lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);   // 字体
```

### 按钮 (Button)

```cpp
lv_obj_t *btn = lv_button_create(parent);
lv_obj_set_size(btn, 100, 42);
lv_obj_set_style_bg_color(btn, lv_color_hex(0x21262D), 0);
lv_obj_add_event_cb(btn, my_callback, LV_EVENT_CLICKED, NULL);

// 按钮内放文字
lv_obj_t *lbl = lv_label_create(btn);
lv_label_set_text(lbl, "点击");
lv_obj_center(lbl);
```

### 开关 (Switch)

```cpp
lv_obj_t *sw = lv_switch_create(parent);
lv_obj_set_size(sw, 70, 35);
lv_obj_add_event_cb(sw, on_switch, LV_EVENT_VALUE_CHANGED, NULL);

// 读取状态
bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
```

### 滑块 (Slider)

```cpp
lv_obj_t *slider = lv_slider_create(parent);
lv_obj_set_width(slider, 200);
lv_slider_set_range(slider, 0, 100);
lv_slider_set_value(slider, 50, LV_ANIM_OFF);
lv_obj_add_event_cb(slider, on_slider, LV_EVENT_VALUE_CHANGED, NULL);

// 读取值
int val = lv_slider_get_value(slider);
```

### 图标 (Symbols)

LVGL 内置图标直接用字符串表示：

```cpp
LV_SYMBOL_PLAY        // ▶
LV_SYMBOL_PAUSE       // ⏸
LV_SYMBOL_LEFT        // ←
LV_SYMBOL_RIGHT       // →
LV_SYMBOL_REFRESH     // ↻
LV_SYMBOL_SETTINGS    // ⚙
LV_SYMBOL_IMAGE       // 🖼
LV_SYMBOL_BLUETOOTH   // 蓝牙图标
LV_SYMBOL_BATTERY_FULL // 电池图标
LV_SYMBOL_HOME        // 🏠
```

用法：直接作为 label 文字

```cpp
lv_label_set_text(lbl, LV_SYMBOL_PLAY " 开始");
```

---

## 五、布局系统

本项目使用 **Flex 布局**（类似 CSS Flexbox），所有容器都用：

```cpp
// 设置为纵向排列
lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

// 设置对齐方式：主轴 | 交叉轴 | 交叉轴（多行时）
lv_obj_set_flex_align(parent,
    LV_FLEX_ALIGN_CENTER,    // 主轴居中
    LV_FLEX_ALIGN_CENTER,    // 交叉轴居中
    LV_FLEX_ALIGN_CENTER);   // 多行时居中

// 间距
lv_obj_set_style_pad_gap(parent, 10, 0);  // 子控件间距 10px
```

### 常用 flex_flow

| 值 | 效果 |
|----|------|
| `LV_FLEX_FLOW_COLUMN` | 纵向排列（从上到下） |
| `LV_FLEX_FLOW_ROW` | 横向排列（从左到右） |
| `LV_FLEX_FLOW_ROW_WRAP` | 横向排列，自动换行 |

### 常用 flex_align

| 值 | 效果 |
|----|------|
| `LV_FLEX_ALIGN_START` | 靠起点 |
| `LV_FLEX_ALIGN_CENTER` | 居中 |
| `LV_FLEX_ALIGN_END` | 靠终点 |
| `LV_FLEX_ALIGN_SPACE_BETWEEN` | 两端对齐 |
| `LV_FLEX_ALIGN_SPACE_EVENLY` | 均匀分布 |

---

## 六、样式系统

### 全局颜色常量（已在 ui_screens.cpp 定义）

```cpp
#define COLOR_BG        lv_color_hex(0x0D1117)  // 深色背景
#define COLOR_HEADER    lv_color_hex(0x161B22)  // 标题栏背景
#define COLOR_TIME      lv_color_hex(0xFFFFFF)  // 白色
#define COLOR_DATE      lv_color_hex(0x8B949E)  // 灰色
#define COLOR_ACCENT    lv_color_hex(0x58A6FF)  // 蓝色强调
#define COLOR_BTN_BG    lv_color_hex(0x21262D)  // 按钮背景
#define COLOR_BTN_TEXT  lv_color_hex(0xC9D1D9)  // 按钮文字
#define COLOR_GREEN     lv_color_hex(0x238636)  // 绿色
#define COLOR_RED       lv_color_hex(0xDA3633)  // 红色
```

### 修改控件样式

```cpp
// 背景色
lv_obj_set_style_bg_color(obj, COLOR_GREEN, 0);

// 背景透明度（0=透明, 255=不透明）
lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);  // 完全透明

// 圆角
lv_obj_set_style_radius(obj, 12, 0);  // 12px 圆角

// 边框
lv_obj_set_style_border_width(obj, 0, 0);  // 无边框

// 内边距
lv_obj_set_style_pad_all(obj, 8, 0);       // 四周 8px
lv_obj_set_style_pad_hor(obj, 12, 0);      // 左右 12px
```

---

## 七、事件系统

### 添加事件监听

```cpp
lv_obj_add_event_cb(obj, callback_function, LV_EVENT_CLICKED, user_data);
```

### 常用事件

| 事件 | 触发时机 |
|------|---------|
| `LV_EVENT_CLICKED` | 按钮被点击（按下+释放） |
| `LV_EVENT_PRESSED` | 手指按下 |
| `LV_EVENT_RELEASED` | 手指释放 |
| `LV_EVENT_VALUE_CHANGED` | 值变化（开关、滑块） |

### 事件回调模板

```cpp
static void my_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);  // 获取事件类型
    lv_obj_t *target = lv_event_get_target(e);     // 获取触发控件

    if (code == LV_EVENT_CLICKED) {
        // 处理点击
    }
}
```

---

## 八、页面导航

### 核心机制

```cpp
// 所有屏幕存储在数组中
static lv_obj_t *screens[SCREEN_COUNT] = {NULL};

// 导航函数：直接加载目标屏幕
void screens_nav(screen_id_t id) {
    lv_screen_load(screens[id]);
}
```

### 导航模式

**直接跳转：**

```cpp
static void nav_timer(lv_event_t *e) {
    (void)e;
    screens_nav(SCREEN_TIMER);  // 直接切换
}
```

**带回调的导航（先执行逻辑再跳转）：**

```cpp
static void nav_with_action(lv_event_t *e) {
    some_function();           // 先执行
    screens_nav(SCREEN_TIMER); // 再跳转
}
```

---

## 九、更新界面数据

**不要在业务逻辑里直接操作 LVGL 控件！** 使用间接更新：

### 方式 1：通过公开函数更新

```cpp
// 在 ui_screens.h 声明
void screens_update_brightness(int percent);

// 在 ui_screens.cpp 实现
void screens_update_brightness(int percent)
{
    if (br_label_value) {
        lv_label_set_text_fmt(br_label_value, "%d%%", percent);
    }
}

// 在 loop.cpp 调用
void brightness_update_cb(lv_timer_t *timer) {
    int val = read_sensor();
    screens_update_brightness(val);
}
```

### 方式 2：通过回调函数（反向控制）

```cpp
// 在 ui_screens.h 声明
void ui_brightness_set_callbacks(void (*set)(int));

// 在 ui_screens.cpp 实现
static void (*cb_set_brightness)(int) = NULL;
void ui_brightness_set_callbacks(void (*set)(int)) {
    cb_set_brightness = set;
}

// 在 slider 回调中调用
static void on_slider_change(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    lv_label_set_text_fmt(br_label_value, "%d%%", val);  // 更新显示
    if (cb_set_brightness) cb_set_brightness(val);         // 执行逻辑
}

// 在 loop.cpp 注册
ui_brightness_set_callbacks(my_brightness_setter);
```

---

## 十、LVGL 定时器

用于周期性更新 UI，比 Arduino `delay()` 更优雅：

```cpp
// 创建定时器：每 1000ms 调用一次 callback
lv_timer_create(my_callback, 1000, NULL);

// 创建定时器并保存引用（之后可修改间隔或删除）
lv_timer_t *timer = lv_timer_create(my_callback, 500, NULL);
lv_timer_set_period(timer, 2000);  // 改为 2000ms
lv_timer_delete(timer);            // 删除定时器
```

---

## 十一、字体配置

当前启用的字体（在 `platformio.ini` 的 build_flags 中配置）：

| 字体 | 状态 | 用途 |
|------|------|------|
| `lv_font_montserrat_14` | ✅ 启用 | 按钮文字 |
| `lv_font_montserrat_16` | ✅ 启用 | 日期、小标签 |
| `lv_font_montserrat_18` | ✅ 启用 | 标题栏 |
| `lv_font_montserrat_20` | ✅ 启用 | 状态文字 |
| `lv_font_montserrat_22` | ✅ 启用 | — |
| `lv_font_montserrat_24` | ✅ 启用 | — |
| `lv_font_montserrat_28` | ✅ 启用 | — |
| `lv_font_montserrat_32` | ✅ 启用 | — |
| `lv_font_montserrat_40` | ✅ 启用 | — |
| `lv_font_montserrat_44` | ✅ 启用 | 大时间显示 |

### 启用新字体

在 `platformio.ini` 的 `build_flags` 中添加：

```
-D LV_FONT_MONTSERRAT_26=1
```

然后代码中使用：

```cpp
lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
```

### 使用中文（自定义字体）

需要将中文字体转换为 C 数组。项目已有 `tools/` 目录下的转换工具。

---

## 十二、添加中文字体

### 步骤 1：用转换工具生成字体文件

```bash
python tools/bdf2lvgl.py -i your_font.bdf -o src/my_font.c -n my_font_16
```

### 步骤 2：在代码中引用

```cpp
// 在文件顶部声明外部字体
extern const lv_font_t my_font_16;

// 使用
lv_obj_set_style_text_font(label, &my_font_16, 0);
```

---

## 十三、调试技巧

### 开启 LVGL 日志

在 `platformio.ini` 的 build_flags 中修改：

```
-D LV_USE_LOG=1
```

代码中设置日志级别：

```cpp
lv_log_set_level(LV_LOG_LEVEL_INFO);  // 或 LV_LOG_LEVEL_TRACE
```

### 常用调试函数

```cpp
// 打印对象树
lv_obj_dump_tree(lv_screen_active(), 0);

// 获取对象尺寸
int w = lv_obj_get_width(obj);
int h = lv_obj_get_height(obj);

// 获取对象坐标
int x = lv_obj_get_x(obj);
int y = lv_obj_get_y(obj);
```

### 内存不足

如果出现内存错误，检查：

1. 缓冲区大小：`SCREEN_W * BUF_LINES * sizeof(lv_color_t)` = 240×40×2 = 19200 字节
2. PSRAM 是否启用：确认 `BOARD_HAS_PSRAM` 已定义
3. 减少 `BUF_LINES`：将 40 改为 20 可节省一半缓冲区内存

---

## 十四、添加新控件的完整模板

将以下代码复制到 `ui_screens.cpp` 即可快速创建新界面：

```cpp
/* ====== 新界面 ====== */

static lv_obj_t *my_label = NULL;  // 需要动态更新的控件用 static 保存

static void nav_my_page(lv_event_t *e) {
    (void)e;
    screens_nav(SCREEN_MY_PAGE);
}

static void nav_back(lv_event_t *e) {
    (void)e;
    screens_nav(SCREEN_WATCHFACE);
}

static void on_my_button(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // 按钮被点击
        lv_label_set_text(my_label, "已点击!");
    }
}

static void create_my_page(void)
{
    lv_obj_t *scr = create_base(COLOR_BG);
    create_header(scr, "我的页面", nav_back);

    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_W, 250);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(content, 15, 0);

    // 标签
    my_label = lv_label_create(content);
    lv_label_set_text(my_label, "Hello LVGL!");
    lv_obj_set_style_text_color(my_label, COLOR_TIME, 0);
    lv_obj_set_style_text_font(my_label, &lv_font_montserrat_24, 0);

    // 按钮
    create_btn(content, "点击我", COLOR_ACCENT, on_my_button);

    screens[SCREEN_MY_PAGE] = scr;
}
```

---

## 十五、关键 API 速查表

| 操作 | API |
|------|-----|
| 创建屏幕 | `lv_obj_create(NULL)` |
| 创建子对象 | `lv_obj_create(parent)` |
| 设置大小 | `lv_obj_set_size(obj, w, h)` |
| 设置位置 | `lv_obj_set_pos(obj, x, y)` |
| 居中 | `lv_obj_center(obj)` |
| 创建标签 | `lv_label_create(parent)` |
| 设置文字 | `lv_label_set_text(obj, "text")` |
| 格式化文字 | `lv_label_set_text_fmt(obj, "%d", val)` |
| 创建按钮 | `lv_button_create(parent)` |
| 创建开关 | `lv_switch_create(parent)` |
| 创建滑块 | `lv_slider_create(parent)` |
| 添加事件 | `lv_obj_add_event_cb(obj, cb, event, data)` |
| 获取事件码 | `lv_event_get_code(e)` |
| 获取目标对象 | `lv_event_get_target(e)` |
| 创建定时器 | `lv_timer_create(cb, period_ms, data)` |
| 加载屏幕 | `lv_screen_load(scr)` |
| 删除对象 | `lv_obj_delete(obj)` |
| 隐藏对象 | `lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN)` |
| 显示对象 | `lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN)` |

---

## 十六、LVGL 官方资源

- **文档**: https://docs.lvgl.io/
- **API 参考**: https://docs.lvgl.io/9.2/
- **示例**: https://github.com/lvgl/lvgl/tree/master/examples
- **SquareLine Studio** (可视化 UI 设计器): https://squareline.io/

---

*最后更新: 2026-09-19*
