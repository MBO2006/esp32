"""
中文字符 → TFT_eSPI 字库转换器
用法：python font_converter.py
依赖：pip install Pillow
"""

from PIL import Image, ImageDraw, ImageFont
import os

# ===== 配置区 =====
FONT_PATH = "C:/Windows/Fonts/msyh.ttc"  # 字体路径（微软雅黑）
FONT_SIZE = 16                             # 字号（像素）
CHARS = "心率步数时间设置电量睡眠闹钟温度日期主页返回功能蓝牙充电"  # 要转换的中文字符
OUTPUT_FILE = "chinese_font.h"             # 输出文件名
BG_COLOR = 0       # 背景色（黑色 = 0）
FG_COLOR = 0xFFFF  # 前景色（白色 = 0xFFFF，RGB565）
# ==================


def rgb888_to_rgb565(r, g, b):
    """RGB888 转 RGB565"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def char_to_bitmap(char, font):
    """把一个字符渲染成 RGB565 位图数组"""
    # 创建临时图片获取字符尺寸
    temp_img = Image.new("RGB", (1, 1))
    temp_draw = ImageDraw.Draw(temp_img)
    bbox = temp_draw.textbbox((0, 0), char, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]

    # 创建图片并绘制字符
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.text((-bbox[0], -bbox[0]), char, font=font, fill=(255, 255, 255))

    # 转为 RGB565 数组
    pixels = []
    for y in range(h):
        for x in range(w):
            r, g, b = img.getpixel((x, y))
            pixels.append(rgb888_to_rgb565(r, g, b))

    return w, h, pixels


def main():
    # 检查字体文件
    if not os.path.exists(FONT_PATH):
        print(f"错误：找不到字体文件 {FONT_PATH}")
        print("请修改 FONT_PATH 为你的字体路径")
        print("  Windows: C:/Windows/Fonts/msyh.ttc (微软雅黑)")
        print("  Windows: C:/Windows/Fonts/simhei.ttf (黑体)")
        return

    # 加载字体
    try:
        font = ImageFont.truetype(FONT_PATH, FONT_SIZE)
    except Exception as e:
        print(f"加载字体失败：{e}")
        return

    # 去重并保持顺序
    unique_chars = list(dict.fromkeys(CHARS))
    print(f"字体：{FONT_PATH}")
    print(f"字号：{FONT_SIZE}px")
    print(f"字符数：{len(unique_chars)}")
    print(f"字符：{''.join(unique_chars)}")
    print()

    # 生成每个字符的位图
    char_data = []
    for ch in unique_chars:
        w, h, pixels = char_to_bitmap(ch, font)
        char_data.append((ch, w, h, pixels))
        print(f"  '{ch}' → {w}x{h}")

    # 写入头文件
    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write("// 自动生成的中文字库\n")
        f.write(f"// 字体: {os.path.basename(FONT_PATH)}, 字号: {FONT_SIZE}px\n")
        f.write(f"// 字符数: {len(unique_chars)}\n")
        f.write(f"// 包含: {CHARS}\n\n")
        f.write("#pragma once\n\n")
        f.write("#include <Arduino.h>\n\n")

        # 写入字符宽度和高度表
        f.write(f"#define FONT_SIZE {FONT_SIZE}\n")
        f.write(f"#define CHAR_COUNT {len(unique_chars)}\n\n")

        # 逐字符写入数据
        for i, (ch, w, h, pixels) in enumerate(char_data):
            safe_name = f"char_{i:03d}"
            f.write(f"// '{ch}' ({w}x{h})\n")
            f.write(f"static const uint16_t {safe_name}[] PROGMEM = {{\n")

            # 每行 16 个像素值
            for j in range(0, len(pixels), 16):
                line = pixels[j:j+16]
                f.write("    " + ", ".join(f"0x{p:04X}" for p in line) + ",\n")

            f.write("};\n\n")

        # 写入字符信息结构体数组
        f.write("struct CharInfo {\n")
        f.write("    uint16_t unicode;  // Unicode 码点\n")
        f.write("    uint8_t  width;    // 字符宽度\n")
        f.write("    uint8_t  height;   // 字符高度\n")
        f.write("    const uint16_t *bitmap; // 位图数据\n")
        f.write("};\n\n")

        f.write("static const CharInfo fontTable[CHAR_COUNT] = {\n")
        for i, (ch, w, h, pixels) in enumerate(char_data):
            code = ord(ch)
            f.write(f"    {{0x{code:04X}, {w}, {h}, char_{i:03d}}},  // '{ch}'\n")
        f.write("};\n\n")

        # 写入查找函数
        f.write("""// 根据 Unicode 查找字符，返回 nullptr 表示未找到
const CharInfo* findChar(uint16_t unicode) {
    for (int i = 0; i < CHAR_COUNT; i++) {
        if (fontTable[i].unicode == unicode) {
            return &fontTable[i];
        }
    }
    return nullptr;
}

// 在屏幕上绘制一个中文字符（需要 TFT_eSPI）
// 用法：drawChineseChar(&tft, 10, 10, '心');
void drawChineseChar(TFT_eSPI *tft, int16_t x, int16_t y, uint16_t unicode) {
    const CharInfo *ch = findChar(unicode);
    if (ch == nullptr) return;
    tft->pushImage(x, y, ch->width, ch->height, ch->bitmap);
}

// 绘制中文字符串
// 用法：drawChineseString(&tft, 10, 10, "心率");
void drawChineseString(TFT_eSPI *tft, int16_t x, int16_t y, const char *str) {
    int16_t cursorX = x;
    while (*str) {
        // 解析 UTF-8
        uint16_t unicode = 0;
        uint8_t c = (uint8_t)*str;
        if (c < 0x80) {
            unicode = c;
            str += 1;
        } else if ((c & 0xE0) == 0xC0) {
            unicode = ((c & 0x1F) << 6) | (str[1] & 0x3F);
            str += 2;
        } else if ((c & 0xF0) == 0xE0) {
            unicode = ((c & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
            str += 3;
        } else {
            str += 1;
            continue;
        }
        const CharInfo *ch = findChar(unicode);
        if (ch) {
            tft->pushImage(cursorX, y, ch->width, ch->height, ch->bitmap);
            cursorX += ch->width;
        }
    }
}
""")

    print(f"\n生成完成！输出文件：{OUTPUT_FILE}")
    print(f"文件大小：{os.path.getsize(OUTPUT_FILE) / 1024:.1f} KB")


if __name__ == "__main__":
    main()
