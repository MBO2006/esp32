"""
中文字库生成器 GUI — 支持增量更新
用法：python font_converter_gui.py
依赖：pip install Pillow
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageDraw, ImageFont, ImageTk
import os
import re


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def char_to_bitmap(char, font):
    temp_img = Image.new("RGB", (1, 1))
    temp_draw = ImageDraw.Draw(temp_img)
    bbox = temp_draw.textbbox((0, 0), char, font=font)
    w = max(bbox[2] - bbox[0], 1)
    h = max(bbox[3] - bbox[1], 1)

    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.text((-bbox[0], -bbox[1]), char, font=font, fill=(255, 255, 255))

    pixels = []
    for y_pos in range(h):
        for x_pos in range(w):
            r, g, b = img.getpixel((x_pos, y_pos))
            pixels.append(rgb888_to_rgb565(r, g, b))

    return w, h, pixels, img


def parse_existing_h(filepath):
    """解析已有的 .h 文件，提取字符信息和位图数据"""
    if not os.path.exists(filepath):
        return None

    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    # 提取字体信息
    font_match = re.search(r"// 字体: (.+?), 字号: (\d+)px", content)
    if not font_match:
        return None
    font_name = font_match.group(1)
    font_size = int(font_match.group(2))

    # 提取字符数据：匹配 cnFontTable 条目
    # {0xXXXX, W, H, cn_char_XXX},  // '字'
    table_pattern = r"\{0x([0-9A-Fa-f]+),\s*(\d+),\s*(\d+),\s*cn_char_(\d+)\},\s*//\s*'(.)'"
    entries = re.findall(table_pattern, content)

    if not entries:
        return None

    char_data = []
    for unicode_hex, w, h, idx, ch in entries:
        w, h = int(w), int(h)

        # 提取对应的位图数据
        bitmap_pattern = rf"cn_char_{idx}\[\]\s*PROGMEM\s*=\s*\{{([^}}]+)\}}"
        bitmap_match = re.search(bitmap_pattern, content)
        pixels = []
        if bitmap_match:
            hex_values = re.findall(r"0x([0-9A-Fa-f]{4})", bitmap_match.group(1))
            pixels = [int(v, 16) for v in hex_values]

        # 从像素重建预览图
        img = Image.new("RGB", (w, h), (0, 0, 0))
        for y_pos in range(h):
            for x_pos in range(w):
                idx_pixel = y_pos * w + x_pos
                if idx_pixel < len(pixels):
                    c = pixels[idx_pixel]
                    r = ((c >> 11) & 0x1F) << 3
                    g = ((c >> 5) & 0x3F) << 2
                    b = (c & 0x1F) << 3
                    img.putpixel((x_pos, y_pos), (r, g, b))

        char_data.append((ch, w, h, pixels, img))

    return {
        "font_name": font_name,
        "font_size": font_size,
        "char_data": char_data,
    }


def generate_header(char_data_with_img, font_size, font_name, output_file):
    """生成 .h 文件，char_data_with_img: [(ch, w, h, pixels, img), ...]"""
    with open(output_file, "w", encoding="utf-8") as f:
        f.write("// 自动生成的中文字库 — 请勿手动修改\n")
        f.write(f"// 字体: {font_name}, 字号: {font_size}px\n")
        f.write(f"// 字符数: {len(char_data_with_img)}\n\n")
        f.write("#pragma once\n\n")
        f.write("#include <Arduino.h>\n")
        f.write("#include <TFT_eSPI.h>\n\n")
        f.write(f"#define CN_FONT_SIZE {font_size}\n")
        f.write(f"#define CN_CHAR_COUNT {len(char_data_with_img)}\n\n")

        for i, item in enumerate(char_data_with_img):
            ch, w, h, pixels, _ = item
            f.write(f"// '{ch}' (U+{ord(ch):04X}) {w}x{h}\n")
            f.write(f"static const uint16_t cn_char_{i:03d}[] PROGMEM = {{\n")
            for j in range(0, len(pixels), 16):
                line = pixels[j:j + 16]
                f.write("    " + ", ".join(f"0x{p:04X}" for p in line) + ",\n")
            f.write("};\n\n")

        f.write("struct CnCharInfo {\n")
        f.write("    uint16_t unicode;\n")
        f.write("    uint8_t  width;\n")
        f.write("    uint8_t  height;\n")
        f.write("    const uint16_t *bitmap;\n")
        f.write("};\n\n")

        f.write("static const CnCharInfo cnFontTable[CN_CHAR_COUNT] = {\n")
        for i, item in enumerate(char_data_with_img):
            ch, w, h, _, _ = item
            f.write(f"    {{0x{ord(ch):04X}, {w}, {h}, cn_char_{i:03d}}},  // '{ch}'\n")
        f.write("};\n\n")

        f.write("""static const CnCharInfo* cnFindChar(uint16_t unicode) {
    for (int i = 0; i < CN_CHAR_COUNT; i++) {
        if (cnFontTable[i].unicode == unicode) return &cnFontTable[i];
    }
    return nullptr;
}

static void cnDrawChar(TFT_eSPI *tft, int16_t x, int16_t y, uint16_t unicode) {
    const CnCharInfo *ch = cnFindChar(unicode);
    if (!ch) return;
    uint16_t idx = 0;
    for (int row = 0; row < ch->height; row++) {
        for (int col = 0; col < ch->width; col++) {
            uint16_t pixel = pgm_read_word(&ch->bitmap[idx++]);
            if (pixel != 0x0000) tft->drawPixel(x + col, y + row, pixel);
        }
    }
}

static void cnDrawString(TFT_eSPI *tft, int16_t x, int16_t y, const char *str) {
    int16_t cx = x;
    while (*str) {
        uint16_t u = 0;
        uint8_t c = (uint8_t)*str;
        if (c < 0x80) { u = c; str += 1; }
        else if ((c & 0xE0) == 0xC0) { u = ((c & 0x1F) << 6) | (str[1] & 0x3F); str += 2; }
        else if ((c & 0xF0) == 0xE0) { u = ((c & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F); str += 3; }
        else { str += 1; continue; }
        const CnCharInfo *ch = cnFindChar(u);
        if (ch) {
            uint16_t idx = 0;
            for (int row = 0; row < ch->height; row++) {
                for (int col = 0; col < ch->width; col++) {
                    uint16_t pixel = pgm_read_word(&ch->bitmap[idx++]);
                    if (pixel != 0x0000) tft->drawPixel(cx + col, y + row, pixel);
                }
            }
            cx += ch->width;
        }
    }
}
""")


def find_system_font():
    candidates = [
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
        "/System/Library/Fonts/PingFang.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
    ]
    for p in candidates:
        if os.path.exists(p):
            return p
    return ""


class FontConverterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("中文字库生成器 — ESP32 手环")
        self.root.geometry("780x700")
        self.root.resizable(True, True)

        self.font_path = tk.StringVar(value=find_system_font())
        self.font_size = tk.IntVar(value=24)
        self.current_scale = 4
        self.tk_images = []

        # 已有字库数据：[(ch, w, h, pixels, img), ...]
        self.existing_chars = {}  # {char: (ch, w, h, pixels, img)}
        self.new_char_data = []   # [(ch, w, h, pixels, img)]
        self.loaded_h_path = ""   # 已加载的 .h 文件路径
        self.loaded_font_size = 0 # 已加载字库的字号

        self.build_ui()

    def build_ui(self):
        # === 字体选择 ===
        frame_font = ttk.LabelFrame(self.root, text="字体设置", padding=10)
        frame_font.pack(fill="x", padx=10, pady=5)

        ttk.Label(frame_font, text="字体文件:").grid(row=0, column=0, sticky="w")
        ttk.Entry(frame_font, textvariable=self.font_path, width=50).grid(row=0, column=1, padx=5, sticky="ew")
        ttk.Button(frame_font, text="浏览...", command=self.browse_font).grid(row=0, column=2)
        frame_font.columnconfigure(1, weight=1)

        ttk.Label(frame_font, text="字号(px):").grid(row=1, column=0, sticky="w", pady=(5, 0))
        ttk.Spinbox(frame_font, from_=10, to=48, textvariable=self.font_size, width=5).grid(row=1, column=1, sticky="w", padx=5, pady=(5, 0))

        # === 已有字库 ===
        frame_existing = ttk.LabelFrame(self.root, text="已有字库（绿色 = 已有，红色 = 新增）", padding=10)
        frame_existing.pack(fill="x", padx=10, pady=5)

        btn_row = ttk.Frame(frame_existing)
        btn_row.pack(fill="x")
        ttk.Button(btn_row, text="📂 加载已有 .h 文件", command=self.load_existing).pack(side="left", padx=5)
        self.existing_label = ttk.Label(btn_row, text="未加载", foreground="gray")
        self.existing_label.pack(side="left", padx=10)

        self.existing_text = tk.Text(frame_existing, height=2, font=("Microsoft YaHei", 11), state="disabled", bg="#1a1a1a", fg="#00ff00")
        self.existing_text.pack(fill="x", pady=(5, 0))

        # === 新增字符输入 ===
        frame_chars = ttk.LabelFrame(self.root, text="输入新增字符（自动去重，已有字符会自动跳过）", padding=10)
        frame_chars.pack(fill="x", padx=10, pady=5)

        self.text_input = tk.Text(frame_chars, height=2, font=("Microsoft YaHei", 12))
        self.text_input.pack(fill="x")
        self.text_input.insert("1.0", "心率步数时间设置电量睡眠闹钟温度主页返回功能蓝牙充电运动距离卡路里")
        self.text_input.bind("<KeyRelease>", self.on_input_change)

        # === 操作栏 ===
        frame_btn = ttk.Frame(self.root, padding=5)
        frame_btn.pack(fill="x", padx=10)

        ttk.Button(frame_btn, text="▶ 预览新增", command=self.preview).pack(side="left", padx=5)
        ttk.Button(frame_btn, text="💾 更新 .h 文件", command=self.generate).pack(side="left", padx=5)

        ttk.Label(frame_btn, text="缩放:").pack(side="left", padx=(20, 5))
        self.scale_var = tk.IntVar(value=4)
        ttk.Scale(frame_btn, from_=1, to=16, variable=self.scale_var,
                  orient="horizontal", length=120, command=self.on_scale_change).pack(side="left")
        self.scale_label = ttk.Label(frame_btn, text="4x", width=3)
        self.scale_label.pack(side="left", padx=2)

        self.status_label = ttk.Label(frame_btn, text="", foreground="green")
        self.status_label.pack(side="right", padx=10)

        # === 预览区 ===
        frame_preview = ttk.LabelFrame(self.root, text="预览", padding=5)
        frame_preview.pack(fill="both", expand=True, padx=10, pady=5)

        canvas_frame = ttk.Frame(frame_preview)
        canvas_frame.pack(fill="both", expand=True)

        self.canvas = tk.Canvas(canvas_frame, bg="#000000", highlightthickness=0)
        scroll_y = ttk.Scrollbar(canvas_frame, orient="vertical", command=self.canvas.yview)
        scroll_x = ttk.Scrollbar(frame_preview, orient="horizontal", command=self.canvas.xview)
        self.canvas.configure(yscrollcommand=scroll_y.set, xscrollcommand=scroll_x.set)

        scroll_y.pack(side="right", fill="y")
        self.canvas.pack(side="top", fill="both", expand=True)
        scroll_x.pack(side="bottom", fill="x")

        self.preview_frame = ttk.Frame(self.canvas)
        self.canvas.create_window((0, 0), window=self.preview_frame, anchor="nw")
        self.preview_frame.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))

    def browse_font(self):
        path = filedialog.askopenfilename(
            title="选择字体文件",
            filetypes=[("字体文件", "*.ttf *.ttc *.otf"), ("所有文件", "*.*")]
        )
        if path:
            self.font_path.set(path)

    def load_existing(self):
        """加载已有的 .h 文件"""
        path = filedialog.askopenfilename(
            title="选择已有的字库 .h 文件",
            filetypes=[("C 头文件", "*.h"), ("所有文件", "*.*")]
        )
        if not path:
            return

        result = parse_existing_h(path)
        if not result:
            messagebox.showerror("错误", "无法解析该文件，不是本工具生成的字库文件")
            return

        self.existing_chars.clear()
        for item in result["char_data"]:
            ch = item[0]
            self.existing_chars[ch] = item

        self.loaded_h_path = path
        self.loaded_font_size = result["font_size"]
        chars_str = "".join(self.existing_chars.keys())
        self.existing_label.config(text=f"已加载 {len(self.existing_chars)} 个字符 — {os.path.basename(path)}", foreground="green")

        self.existing_text.config(state="normal")
        self.existing_text.delete("1.0", "end")
        self.existing_text.insert("1.0", chars_str)
        self.existing_text.config(state="disabled")

        self.font_size.set(result["font_size"])
        self.status_label.config(text=f"已加载字库: {len(self.existing_chars)} 个字符", foreground="blue")

    def on_input_change(self, event=None):
        """输入变化时实时更新状态"""
        text = self.text_input.get("1.0", "end").strip()
        all_chars = list(dict.fromkeys(text))
        new_chars = [ch for ch in all_chars if ch not in self.existing_chars]
        skip_chars = [ch for ch in all_chars if ch in self.existing_chars]

        parts = []
        if new_chars:
            parts.append(f"新增 {len(new_chars)} 个")
        if skip_chars:
            parts.append(f"跳过 {len(skip_chars)} 个已有")
        if parts:
            self.status_label.config(text="，".join(parts), foreground="blue")

    def get_chars(self):
        text = self.text_input.get("1.0", "end").strip()
        return list(dict.fromkeys(text))

    def on_scale_change(self, value):
        self.current_scale = int(float(value))
        self.scale_label.config(text=f"{self.current_scale}x")
        self.refresh_preview()

    def refresh_preview(self):
        for w in self.preview_frame.winfo_children():
            w.destroy()
        self.tk_images.clear()

        scale = self.current_scale
        col = 0
        row = 0
        max_cols = max(1, 700 // (self.font_size.get() * scale + 20))

        # 先显示已有字符（绿色=已有，橙色=字号变更重新生成）
        for ch, item in self.existing_chars.items():
            _, w, h, _, img = item
            self._add_char_preview(ch, w, h, img, scale, row, col, "#00aa00", "已有")
            col += 1
            if col >= max_cols:
                col = 0
                row += 1

        # 再显示新增字符（红色边框）
        for item in self.new_char_data:
            ch, w, h, _, img = item
            self._add_char_preview(ch, w, h, img, scale, row, col, "#ff4444", "新增")
            col += 1
            if col >= max_cols:
                col = 0
                row += 1

    def _add_char_preview(self, ch, w, h, img, scale, row, col, border_color, tag):
        preview_img = img.resize((w * scale, h * scale), Image.NEAREST)
        tk_img = ImageTk.PhotoImage(preview_img)
        self.tk_images.append(tk_img)

        frame = tk.Frame(self.preview_frame, bg=border_color, bd=2)
        frame.grid(row=row, column=col, padx=3, pady=3)
        tk.Label(frame, image=tk_img, bg="#000000").pack(padx=2, pady=2)
        tk.Label(frame, text=f"{ch} {w}×{h}\n[{tag}]", font=("", 7),
                 fg=border_color, bg="#111111").pack(pady=(0, 2))

    def preview(self):
        self.new_char_data.clear()

        font_path = self.font_path.get()
        font_size = self.font_size.get()

        if not font_path or not os.path.exists(font_path):
            messagebox.showerror("错误",
                f"找不到字体文件:\n{font_path}\n\n请点击「浏览」选择一个中文字体文件")
            return

        try:
            font = ImageFont.truetype(font_path, font_size)
        except Exception as e:
            messagebox.showerror("错误", f"加载字体失败:\n{e}")
            return

        chars = self.get_chars()
        if not chars:
            messagebox.showwarning("提示", "请输入至少一个字符")
            return

        # 检测字号是否变化
        size_changed = (self.loaded_font_size > 0 and font_size != self.loaded_font_size)
        regenerated = 0

        if size_changed and self.existing_chars:
            for ch in list(self.existing_chars.keys()):
                w, h, pixels, img = char_to_bitmap(ch, font)
                self.existing_chars[ch] = (ch, w, h, pixels, img)
                regenerated += 1
            self.loaded_font_size = font_size

        skipped = 0
        for ch in chars:
            if ch in self.existing_chars:
                skipped += 1
                continue
            w, h, pixels, img = char_to_bitmap(ch, font)
            self.new_char_data.append((ch, w, h, pixels, img))

        total = len(self.existing_chars) + len(self.new_char_data)
        status_parts = [f"总计 {total} 个字符"]
        if regenerated:
            status_parts.append(f"重新生成 {regenerated} 个 → {font_size}px")
        elif self.existing_chars:
            status_parts.append(f"已有 {len(self.existing_chars)}")
        if self.new_char_data:
            status_parts.append(f"新增 {len(self.new_char_data)}")
        if skipped and not size_changed:
            status_parts.append(f"跳过 {skipped}")
        self.status_label.config(text="，".join(status_parts), foreground="blue")

        self.refresh_preview()

    def generate(self):
        # 如果没有预览过，先预览
        if not self.new_char_data and not self.existing_chars:
            self.preview()
            if not self.new_char_data and not self.existing_chars:
                return

        # 确定输出路径
        if self.loaded_h_path:
            output = self.loaded_h_path
        else:
            output = filedialog.asksaveasfilename(
                title="保存 .h 文件",
                defaultextension=".h",
                filetypes=[("C 头文件", "*.h")],
                initialfile="chinese_font.h"
            )
            if not output:
                return

        # 合并：已有字符 + 新增字符
        merged = []
        for ch in self.existing_chars:
            merged.append(self.existing_chars[ch])
        for item in self.new_char_data:
            merged.append(item)

        font_name = os.path.basename(self.font_path.get())
        generate_header(merged, self.font_size.get(), font_name, output)

        # 更新已有字符列表
        for item in merged:
            self.existing_chars[item[0]] = item
        self.new_char_data.clear()

        self.loaded_h_path = output
        file_size = os.path.getsize(output) / 1024

        self.existing_label.config(
            text=f"已加载 {len(self.existing_chars)} 个字符 — {os.path.basename(output)}",
            foreground="green")
        self.existing_text.config(state="normal")
        self.existing_text.delete("1.0", "end")
        self.existing_text.insert("1.0", "".join(self.existing_chars.keys()))
        self.existing_text.config(state="disabled")

        self.status_label.config(
            text=f"✓ 已更新: {os.path.basename(output)} ({file_size:.1f} KB) 共 {len(merged)} 个字符",
            foreground="green")

        self.refresh_preview()

        messagebox.showinfo("完成",
            f"字库已更新:\n{output}\n\n"
            f"文件大小: {file_size:.1f} KB\n"
            f"总字符数: {len(merged)}")


def main():
    root = tk.Tk()
    FontConverterApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
