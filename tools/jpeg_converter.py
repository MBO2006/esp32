"""
图片 → C 数组转换器（用于 TFT_eSPI + JPEGDecoder）

支持格式：JPEG、PNG、BMP、GIF、TIFF
非 JPEG 格式会自动转换为 JPEG 后再生成数组。

用法：
  python jpeg_converter.py photo.jpg
  python jpeg_converter.py photo.png -n myImage -o src/img_photo.h
  python jpeg_converter.py logo.jpg --name logoImg --output src/img_logo.h

依赖：pip install Pillow（仅非 JPEG 格式需要）
"""

import argparse
import io
import os
import sys


def convert_to_jpeg(input_path: str) -> bytes:
    """将非 JPEG 图片转换为 JPEG 字节流"""
    try:
        from PIL import Image
    except ImportError:
        print("错误：处理 PNG/BMP 等格式需要 Pillow 库")
        print("请运行：pip install Pillow")
        sys.exit(1)

    img = Image.open(input_path)

    # 如果有透明通道（PNG），合成白色背景
    if img.mode in ("RGBA", "LA", "PA"):
        bg = Image.new("RGB", img.size, (255, 255, 255))
        if img.mode == "RGBA":
            bg.paste(img, mask=img.split()[3])  # 用 alpha 通道做蒙版
        else:
            bg.paste(img)
        img = bg
    elif img.mode != "RGB":
        img = img.convert("RGB")

    # 转为 JPEG 字节流
    buf = io.BytesIO()
    img.save(buf, format="JPEG", quality=85)
    return buf.getvalue()


def load_jpeg_data(input_path: str) -> tuple[bytes, str]:
    """
    加载图片数据，返回 (jpeg_bytes, 来源描述)
    JPEG 直接读取，其他格式先转换
    """
    ext = os.path.splitext(input_path)[1].lower()

    with open(input_path, "rb") as f:
        header = f.read(8)

    if header[:2] == b'\xFF\xD8':
        # JPEG 文件，直接读取
        with open(input_path, "rb") as f:
            data = f.read()
        return data, os.path.basename(input_path)

    elif header[:8] == b'\x89PNG\r\n\x1a\n':
        print(f"检测到 PNG 格式，正在转换为 JPEG...")
        data = convert_to_jpeg(input_path)
        print(f"  PNG → JPEG 转换完成 ({len(data) / 1024:.1f} KB)")
        return data, f"{os.path.basename(input_path)} (转为JPEG)"

    elif header[:2] == b'BM':
        print(f"检测到 BMP 格式，正在转换为 JPEG...")
        data = convert_to_jpeg(input_path)
        print(f"  BMP → JPEG 转换完成 ({len(data) / 1024:.1f} KB)")
        return data, f"{os.path.basename(input_path)} (转为JPEG)"

    else:
        # 尝试用 Pillow 打开（可能是 GIF、TIFF 等）
        try:
            print(f"检测到图片格式，正在转换为 JPEG...")
            data = convert_to_jpeg(input_path)
            print(f"  转换完成 ({len(data) / 1024:.1f} KB)")
            return data, f"{os.path.basename(input_path)} (转为JPEG)"
        except Exception:
            print(f"错误：无法识别 {input_path} 的格式")
            print("支持的格式：JPEG (.jpg/.jpeg)、PNG (.png)、BMP (.bmp)、GIF、TIFF")
            sys.exit(1)


def jpeg_to_c_array(jpeg_data: bytes, var_name: str, source_desc: str) -> str:
    """将 JPEG 字节数据生成 C 头文件内容"""
    file_size = len(jpeg_data)

    lines = []
    lines.append("// 自动生成的 JPEG 数据 — 请勿手动修改")
    lines.append(f"// 来源: {source_desc}")
    lines.append(f"// 大小: {file_size} 字节 ({file_size / 1024:.1f} KB)")
    lines.append("//")
    lines.append(f"// 用法: drawJpeg({var_name}, sizeof({var_name}), x, y);")
    lines.append("")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <Arduino.h>")
    lines.append("")
    lines.append(f"const uint8_t {var_name}[] PROGMEM = {{")

    # 每行 16 字节
    for i in range(0, file_size, 16):
        chunk = jpeg_data[i:i + 16]
        hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
        comma = "," if i + 16 < file_size else ""
        lines.append(f"    {hex_vals}{comma}")

    lines.append("};")
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="图片 → C 数组转换器（JPEG/PNG/BMP → PROGMEM 数组）",
        epilog="示例:\n"
               "  python jpeg_converter.py photo.jpg\n"
               "  python jpeg_converter.py photo.png -n myPhoto -o src/img_photo.h",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("input", help="输入的图片文件路径（JPEG/PNG/BMP）")
    parser.add_argument("-n", "--name", default=None,
                        help="C 变量名（默认从文件名自动生成）")
    parser.add_argument("-o", "--output", default=None,
                        help="输出 .h 文件路径（默认生成到 src/ 目录）")

    args = parser.parse_args()

    # 检查输入文件
    if not os.path.exists(args.input):
        print(f"错误：找不到文件 {args.input}")
        sys.exit(1)

    # 加载图片数据（自动处理 PNG → JPEG 转换）
    jpeg_data, source_desc = load_jpeg_data(args.input)

    # 生成变量名
    if args.name:
        var_name = args.name
    else:
        base = os.path.splitext(os.path.basename(args.input))[0]
        safe = "".join(c if c.isalnum() else "_" for c in base)
        if safe[0].isdigit():
            safe = "_" + safe
        var_name = f"img_{safe}"

    # 生成输出路径
    if args.output:
        output_path = args.output
    else:
        output_path = f"src/{var_name}.h"

    # 生成 C 头文件内容
    content = jpeg_to_c_array(jpeg_data, var_name, source_desc)

    # 确保输出目录存在
    out_dir = os.path.dirname(output_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    # 写入文件
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"✅ 转换完成！")
    print(f"   输入: {args.input}")
    print(f"   输出: {output_path} ({os.path.getsize(output_path) / 1024:.1f} KB)")
    print(f"   变量: {var_name}")
    print(f"")
    print(f"   在代码中使用:")
    print(f"   1. display.cpp 顶部添加:  #include \"{os.path.basename(output_path)}\"")
    print(f"   2. 想显示的地方调用:      drawJpeg({var_name}, sizeof({var_name}), 0, 0);")


if __name__ == "__main__":
    main()
