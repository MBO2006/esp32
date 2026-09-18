"""
图片 → C 数组转换器 GUI（用于 TFT_eSPI + JPEGDecoder）
支持 JPEG、PNG、BMP、GIF 等格式，支持缩放分辨率，非 JPEG 自动转换后生成 PROGMEM 数组。

用法：python jpeg_converter_gui.py
依赖：pip install Pillow
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageTk
import io
import os
import re


class ImageConverterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("图片 → C 数组转换器 — ESP32 TFT")
        self.root.geometry("700x750")
        self.root.resizable(True, True)

        self.input_path = tk.StringVar()
        self.var_name = tk.StringVar(value="img_photo")
        self.output_path = tk.StringVar(value="src/img_photo.h")
        self.jpeg_quality = tk.IntVar(value=85)

        # 缩放相关
        self.resize_mode = tk.StringVar(value="fit_screen")  # no_resize / fit_screen / custom
        self.custom_w = tk.IntVar(value=240)
        self.custom_h = tk.IntVar(value=320)
        self.keep_ratio = tk.BooleanVar(value=True)

        self.original_img = None   # PIL Image（原始）
        self.resized_img = None    # PIL Image（缩放后）
        self.jpeg_data = None      # 转换后的 JPEG 字节
        self.tk_preview = None     # 预览用的 PhotoImage

        self.build_ui()

    def build_ui(self):
        # === 文件选择 ===
        frame_file = ttk.LabelFrame(self.root, text="选择图片", padding=10)
        frame_file.pack(fill="x", padx=10, pady=5)

        ttk.Label(frame_file, text="图片文件:").grid(row=0, column=0, sticky="w")
        ttk.Entry(frame_file, textvariable=self.input_path, width=50).grid(row=0, column=1, padx=5, sticky="ew")
        ttk.Button(frame_file, text="浏览...", command=self.browse_image).grid(row=0, column=2)
        frame_file.columnconfigure(1, weight=1)

        ttk.Label(frame_file, text="支持 JPEG / PNG / BMP / GIF 等常见格式",
                  foreground="gray").grid(row=1, column=1, sticky="w", pady=(3, 0))

        # === 缩放设置 ===
        frame_resize = ttk.LabelFrame(self.root, text="缩放设置（ILI9341 屏幕: 240×320）", padding=10)
        frame_resize.pack(fill="x", padx=10, pady=5)

        ttk.Radiobutton(frame_resize, text="不缩放（原始尺寸）",
                        variable=self.resize_mode, value="no_resize",
                        command=self.on_resize_mode_change).grid(row=0, column=0, columnspan=4, sticky="w")

        ttk.Radiobutton(frame_resize, text="适应屏幕（缩放到 240×320 以内，保持比例）",
                        variable=self.resize_mode, value="fit_screen",
                        command=self.on_resize_mode_change).grid(row=1, column=0, columnspan=4, sticky="w")

        custom_frame = ttk.Frame(frame_resize)
        custom_frame.grid(row=2, column=0, columnspan=4, sticky="w")

        ttk.Radiobutton(custom_frame, text="自定义尺寸:",
                        variable=self.resize_mode, value="custom",
                        command=self.on_resize_mode_change).pack(side="left")

        ttk.Label(custom_frame, text="宽").pack(side="left", padx=(15, 2))
        ttk.Spinbox(custom_frame, from_=1, to=1024, textvariable=self.custom_w,
                     width=5).pack(side="left")
        ttk.Label(custom_frame, text="高").pack(side="left", padx=(10, 2))
        ttk.Spinbox(custom_frame, from_=1, to=1024, textvariable=self.custom_h,
                     width=5).pack(side="left")

        ttk.Checkbutton(custom_frame, text="保持比例", variable=self.keep_ratio).pack(side="left", padx=(15, 0))

        # 常用尺寸快捷按钮
        preset_frame = ttk.Frame(frame_resize)
        preset_frame.grid(row=3, column=0, columnspan=4, sticky="w", pady=(5, 0))
        ttk.Label(preset_frame, text="快捷:").pack(side="left")
        for label, w, h in [("全屏 240×320", 240, 320), ("半屏 120×160", 120, 160),
                             ("图标 48×48", 48, 48), ("图标 32×32", 32, 32)]:
            ttk.Button(preset_frame, text=label,
                       command=lambda ww=w, hh=h: self.apply_preset(ww, hh)).pack(side="left", padx=3)

        # === 转换设置 ===
        frame_setting = ttk.LabelFrame(self.root, text="转换设置", padding=10)
        frame_setting.pack(fill="x", padx=10, pady=5)

        ttk.Label(frame_setting, text="C 变量名:").grid(row=0, column=0, sticky="w")
        ttk.Entry(frame_setting, textvariable=self.var_name, width=30).grid(row=0, column=1, padx=5, sticky="w")

        ttk.Label(frame_setting, text="输出路径:").grid(row=1, column=0, sticky="w", pady=(5, 0))
        path_frame = ttk.Frame(frame_setting)
        path_frame.grid(row=1, column=1, columnspan=2, sticky="ew", pady=(5, 0))
        ttk.Entry(path_frame, textvariable=self.output_path, width=40).pack(side="left", fill="x", expand=True)
        ttk.Button(path_frame, text="选择...", command=self.browse_output).pack(side="left", padx=(5, 0))
        frame_setting.columnconfigure(1, weight=1)

        ttk.Label(frame_setting, text="JPEG 质量:").grid(row=2, column=0, sticky="w", pady=(5, 0))
        quality_frame = ttk.Frame(frame_setting)
        quality_frame.grid(row=2, column=1, sticky="w", pady=(5, 0))
        ttk.Scale(quality_frame, from_=10, to=100, variable=self.jpeg_quality,
                  orient="horizontal", length=200, command=self.on_quality_change).pack(side="left")
        self.quality_label = ttk.Label(quality_frame, text="85%")
        self.quality_label.pack(side="left", padx=5)

        # === 预览区 ===
        frame_preview = ttk.LabelFrame(self.root, text="预览", padding=10)
        frame_preview.pack(fill="both", expand=True, padx=10, pady=5)

        self.preview_label = ttk.Label(frame_preview, text="请选择一张图片",
                                       anchor="center", foreground="gray")
        self.preview_label.pack(fill="both", expand=True)

        # === 信息栏 ===
        self.info_label = ttk.Label(self.root, text="", foreground="gray", padding=(15, 0))
        self.info_label.pack(fill="x")

        # === 操作栏 ===
        frame_btn = ttk.Frame(self.root, padding=10)
        frame_btn.pack(fill="x")

        ttk.Button(frame_btn, text="🔄 转换并预览", command=self.convert).pack(side="left", padx=5)
        ttk.Button(frame_btn, text="💾 生成 .h 文件", command=self.generate).pack(side="left", padx=5)

        self.status_label = ttk.Label(frame_btn, text="", foreground="green")
        self.status_label.pack(side="right", padx=10)

    def apply_preset(self, w, h):
        """应用预设尺寸"""
        self.resize_mode.set("custom")
        self.custom_w.set(w)
        self.custom_h.set(h)
        self.on_resize_mode_change()

    def on_resize_mode_change(self):
        """缩放模式变化时，如果有图片则刷新预览"""
        if self.original_img is not None:
            self.show_preview(self.original_img)

    def browse_image(self):
        path = filedialog.askopenfilename(
            title="选择图片文件",
            filetypes=[
                ("图片文件", "*.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp"),
                ("JPEG", "*.jpg *.jpeg"),
                ("PNG", "*.png"),
                ("BMP", "*.bmp"),
                ("所有文件", "*.*")
            ]
        )
        if path:
            self.input_path.set(path)

            # 自动生成变量名和输出路径
            base = os.path.splitext(os.path.basename(path))[0]
            safe = "".join(c if c.isalnum() else "_" for c in base)
            if safe[0].isdigit():
                safe = "_" + safe
            var = f"img_{safe}"
            self.var_name.set(var)
            self.output_path.set(f"src/{var}.h")

            # 加载并预览
            self.load_and_preview(path)

    def browse_output(self):
        path = filedialog.asksaveasfilename(
            title="保存 .h 文件",
            defaultextension=".h",
            filetypes=[("C 头文件", "*.h")],
            initialfile=os.path.basename(self.output_path.get())
        )
        if path:
            self.output_path.set(path)

    def on_quality_change(self, value):
        self.quality_label.config(text=f"{int(float(value))}%")

    def load_and_preview(self, path):
        """加载图片并显示预览"""
        try:
            self.original_img = Image.open(path)
        except Exception as e:
            messagebox.showerror("错误", f"无法打开图片:\n{e}")
            return

        # 显示原始图片信息
        w, h = self.original_img.size
        fmt = self.original_img.format or "未知"
        mode = self.original_img.mode
        file_size = os.path.getsize(path)
        self.info_label.config(
            text=f"原始: {fmt}  {w}×{h}  {mode}  {file_size / 1024:.1f} KB"
        )

        self.show_preview(self.original_img)
        self.status_label.config(text="已加载图片，点击「转换并预览」或直接「生成 .h 文件」", foreground="blue")

    def get_resized_image(self, img: Image.Image) -> Image.Image:
        """根据缩放设置返回缩放后的图片"""
        mode = self.resize_mode.get()
        w, h = img.size

        if mode == "no_resize":
            return img.copy()

        elif mode == "fit_screen":
            # 缩放到 240×320 以内，保持比例
            ratio = min(240 / w, 320 / h, 1.0)
            if ratio >= 1.0:
                return img.copy()  # 已经够小，不需要缩放
            new_w = int(w * ratio)
            new_h = int(h * ratio)
            return img.resize((new_w, new_h), Image.LANCZOS)

        elif mode == "custom":
            target_w = self.custom_w.get()
            target_h = self.custom_h.get()

            if self.keep_ratio.get():
                # 保持比例，缩放到不超过目标尺寸
                ratio = min(target_w / w, target_h / h)
                new_w = int(w * ratio)
                new_h = int(h * ratio)
            else:
                new_w = target_w
                new_h = target_h

            return img.resize((new_w, new_h), Image.LANCZOS)

        return img.copy()

    def show_preview(self, img: Image.Image):
        """显示预览图（根据缩放设置）"""
        self.resized_img = self.get_resized_image(img)
        rw, rh = self.resized_img.size
        ow, oh = img.size

        # 更新信息栏，显示缩放信息
        mode = self.resize_mode.get()
        file_size = os.path.getsize(self.input_path.get()) if self.input_path.get() else 0

        if mode == "no_resize" or (rw == ow and rh == oh):
            resize_info = "不缩放"
        else:
            resize_info = f"缩放: {ow}×{oh} → {rw}×{rh}"

        self.info_label.config(
            text=f"原始: {ow}×{oh}  |  {resize_info}  |  文件: {file_size / 1024:.1f} KB"
        )

        # 缩放预览图用于 GUI 显示（最大 320×400）
        max_preview = 320
        ratio = min(max_preview / rw, max_preview / rh, 1.0)
        preview_w = int(rw * ratio)
        preview_h = int(rh * ratio)
        preview_img = self.resized_img.resize((preview_w, preview_h), Image.LANCZOS)

        self.tk_preview = ImageTk.PhotoImage(preview_img)
        self.preview_label.config(image=self.tk_preview, text="")

    def convert_to_jpeg(self, img: Image.Image) -> bytes:
        """将 PIL Image 转换为 JPEG 字节流"""
        # 处理透明通道
        if img.mode in ("RGBA", "LA", "PA"):
            bg = Image.new("RGB", img.size, (255, 255, 255))
            if img.mode == "RGBA":
                bg.paste(img, mask=img.split()[3])
            else:
                bg.paste(img)
            img = bg
        elif img.mode != "RGB":
            img = img.convert("RGB")

        buf = io.BytesIO()
        img.save(buf, format="JPEG", quality=self.jpeg_quality.get())
        return buf.getvalue()

    def convert(self):
        """转换图片为 JPEG 字节并显示信息"""
        if not self.input_path.get() or not os.path.exists(self.input_path.get()):
            messagebox.showwarning("提示", "请先选择一张图片")
            return

        if self.original_img is None:
            self.load_and_preview(self.input_path.get())
            if self.original_img is None:
                return

        # 先缩放
        self.resized_img = self.get_resized_image(self.original_img)
        self.jpeg_data = self.convert_to_jpeg(self.resized_img)
        jpeg_size = len(self.jpeg_data)
        rw, rh = self.resized_img.size
        ow, oh = self.original_img.size

        # 估算 ESP32 占用
        flash_kb = jpeg_size / 1024
        esp32_flash_pct = jpeg_size / 3342336 * 100

        resize_str = f"{ow}×{oh}" if (rw == ow and rh == oh) else f"{ow}×{oh} → {rw}×{rh}"

        self.info_label.config(
            text=f"尺寸: {resize_str}  |  JPEG: {jpeg_size} 字节 ({flash_kb:.1f} KB)  |  "
                 f"Flash: {esp32_flash_pct:.2f}%  |  "
                 f"可放约 {int(3342336 * 0.5 / jpeg_size)} 张"
        )

        self.status_label.config(
            text=f"✓ 转换完成: {flash_kb:.1f} KB（质量 {self.jpeg_quality.get()}%，{rw}×{rh}）",
            foreground="green"
        )

    def generate(self):
        """生成 .h 文件"""
        if not self.input_path.get() or not os.path.exists(self.input_path.get()):
            messagebox.showwarning("提示", "请先选择一张图片")
            return

        if self.original_img is None:
            self.load_and_preview(self.input_path.get())
            if self.original_img is None:
                return

        var_name = self.var_name.get().strip()
        if not var_name:
            messagebox.showwarning("提示", "请输入 C 变量名")
            return

        # 检查变量名合法性
        if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', var_name):
            messagebox.showerror("错误",
                f"变量名 \"{var_name}\" 不合法\n\n"
                "C 变量名只能包含字母、数字和下划线，且不能以数字开头")
            return

        output_path = self.output_path.get().strip()
        if not output_path:
            messagebox.showwarning("提示", "请设置输出路径")
            return

        # 先缩放，再转 JPEG
        self.resized_img = self.get_resized_image(self.original_img)
        self.jpeg_data = self.convert_to_jpeg(self.resized_img)
        jpeg_size = len(self.jpeg_data)
        rw, rh = self.resized_img.size
        ow, oh = self.original_img.size

        source_desc = os.path.basename(self.input_path.get())
        ext = os.path.splitext(self.input_path.get())[1].lower()
        if ext not in (".jpg", ".jpeg"):
            source_desc += " (转为JPEG)"

        if rw != ow or rh != oh:
            source_desc += f" [{ow}×{oh} → {rw}×{rh}]"

        # 生成 C 头文件内容
        lines = []
        lines.append("// 自动生成的 JPEG 数据 — 请勿手动修改")
        lines.append(f"// 来源: {source_desc}")
        lines.append(f"// 尺寸: {rw} x {rh}")
        lines.append(f"// 大小: {jpeg_size} 字节 ({jpeg_size / 1024:.1f} KB)")
        lines.append(f"// JPEG 质量: {self.jpeg_quality.get()}%")
        lines.append("//")
        lines.append(f"// 用法: drawJpeg({var_name}, sizeof({var_name}), x, y);")
        lines.append("")
        lines.append("#pragma once")
        lines.append("")
        lines.append("#include <Arduino.h>")
        lines.append("")
        lines.append(f"const uint8_t {var_name}[] PROGMEM = {{")

        for i in range(0, jpeg_size, 16):
            chunk = self.jpeg_data[i:i + 16]
            hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
            comma = "," if i + 16 < jpeg_size else ""
            lines.append(f"    {hex_vals}{comma}")

        lines.append("};")
        lines.append("")

        content = "\n".join(lines)

        # 确保输出目录存在
        out_dir = os.path.dirname(output_path)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)

        # 写入文件
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(content)

        file_size = os.path.getsize(output_path) / 1024
        self.status_label.config(
            text=f"✓ 已生成: {os.path.basename(output_path)} ({file_size:.1f} KB)",
            foreground="green"
        )

        messagebox.showinfo("完成",
            f"图片已转换为 C 数组！\n\n"
            f"输出文件: {output_path}\n"
            f"图片尺寸: {rw} × {rh}\n"
            f"文件大小: {file_size:.1f} KB\n"
            f"变量名: {var_name}\n\n"
            f"使用方法:\n"
            f"1. display.cpp 顶部添加:\n"
            f"   #include \"{os.path.basename(output_path)}\"\n\n"
            f"2. 想显示的地方调用:\n"
            f"   drawJpeg({var_name}, sizeof({var_name}), 0, 0);"
        )


def main():
    root = tk.Tk()
    ImageConverterApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
