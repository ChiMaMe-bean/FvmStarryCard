#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复PNG图片中的sRGB配置文件问题的脚本
使用Pillow库重新保存PNG文件，移除有问题的sRGB配置文件
"""

import os
import sys
from pathlib import Path

try:
    from PIL import Image
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False

def fix_png_srgb(png_path):
    """修复单个PNG文件的sRGB配置文件"""
    if not PIL_AVAILABLE:
        print("❌ 需要安装Pillow库: pip install Pillow")
        return False
    
    try:
        # 打开PNG图片
        with Image.open(png_path) as img:
            # 移除所有ICC配置文件信息
            if 'icc_profile' in img.info:
                print(f"  移除ICC配置文件: {png_path}")
                del img.info['icc_profile']
            
            # 重新保存PNG文件，不包含ICC配置文件
            img.save(png_path, 'PNG', optimize=True)
            return True
            
    except Exception as e:
        print(f"❌ 处理失败 {png_path}: {e}")
        return False

def scan_and_fix_png_files(directory):
    """扫描目录并修复所有PNG文件"""
    directory_path = Path(directory)
    
    if not directory_path.exists():
        print(f"❌ 目录不存在: {directory}")
        return
    
    # 查找所有PNG文件
    png_files = list(directory_path.rglob("*.png"))
    
    if not png_files:
        print(f"📂 目录中没有找到PNG文件: {directory}")
        return
    
    print(f"🔍 找到 {len(png_files)} 个PNG文件")
    print("-" * 50)
    
    success_count = 0
    for png_file in png_files:
        print(f"处理: {png_file.relative_to(directory_path)}")
        if fix_png_srgb(png_file):
            success_count += 1
    
    print("-" * 50)
    print(f"✅ 成功处理: {success_count}/{len(png_files)} 个文件")

def main():
    """主函数"""
    print("🛠️  PNG图片sRGB配置文件修复工具")
    print("=" * 50)
    
    if not PIL_AVAILABLE:
        print("❌ 缺少依赖库！")
        print("请安装Pillow库:")
        print("  pip install Pillow")
        print("或者:")
        print("  conda install pillow")
        return
    
    # 项目根目录
    project_root = Path(__file__).parent.parent
    
    # 扫描并修复resources/images目录
    images_dir = project_root / "resources" / "images"
    if images_dir.exists():
        print(f"📂 处理目录: {images_dir}")
        scan_and_fix_png_files(images_dir)
    
    # 扫描并修复resources/qrc/images目录（如果存在）
    qrc_images_dir = project_root / "resources" / "qrc" / "images"
    if qrc_images_dir.exists():
        print(f"\n📂 处理目录: {qrc_images_dir}")
        scan_and_fix_png_files(qrc_images_dir)
    
    print("\n🎉 PNG修复完成！")
    print("重新编译项目后，sRGB警告应该消失。")

if __name__ == "__main__":
    main() 