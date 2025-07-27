#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
检查PNG文件中的sRGB配置文件问题
无需外部依赖，通过读取PNG文件头来检测问题
"""

import os
from pathlib import Path

def check_png_srgb_chunk(png_path):
    """检查PNG文件是否包含有问题的sRGB块"""
    try:
        with open(png_path, 'rb') as f:
            # 读取PNG文件头
            header = f.read(8)
            if header != b'\x89PNG\r\n\x1a\n':
                return False, "不是有效的PNG文件"
            
            # 检查各个块
            has_srgb = False
            has_iccp = False
            
            while True:
                # 读取块长度（4字节大端序）
                length_bytes = f.read(4)
                if len(length_bytes) < 4:
                    break
                
                length = int.from_bytes(length_bytes, 'big')
                
                # 读取块类型（4字节）
                chunk_type = f.read(4)
                if len(chunk_type) < 4:
                    break
                
                chunk_name = chunk_type.decode('ascii', errors='ignore')
                
                # 检查是否是sRGB或iCCP块
                if chunk_name == 'sRGB':
                    has_srgb = True
                elif chunk_name == 'iCCP':
                    has_iccp = True
                
                # 跳过块数据和CRC（length + 4字节）
                f.seek(length + 4, 1)
                
                # 如果到达IEND块，结束
                if chunk_name == 'IEND':
                    break
            
            # 判断是否有问题
            if has_srgb and has_iccp:
                return True, "同时包含sRGB和iCCP块"
            elif has_iccp:
                return True, "包含iCCP块（可能导致警告）"
            else:
                return False, "无问题"
                
    except Exception as e:
        return False, f"读取失败: {e}"

def scan_png_files(directory):
    """扫描目录中的PNG文件"""
    directory_path = Path(directory)
    
    if not directory_path.exists():
        print(f"❌ 目录不存在: {directory}")
        return
    
    # 查找所有PNG文件
    png_files = list(directory_path.rglob("*.png"))
    
    if not png_files:
        print(f"📂 目录中没有找到PNG文件: {directory}")
        return
    
    print(f"🔍 检查 {len(png_files)} 个PNG文件")
    print("-" * 70)
    
    problem_files = []
    
    for png_file in png_files:
        has_problem, reason = check_png_srgb_chunk(png_file)
        relative_path = png_file.relative_to(directory_path)
        
        if has_problem:
            print(f"⚠️  {relative_path} - {reason}")
            problem_files.append((png_file, reason))
        else:
            print(f"✅ {relative_path} - {reason}")
    
    print("-" * 70)
    
    if problem_files:
        print(f"❌ 发现 {len(problem_files)} 个有问题的PNG文件:")
        for file_path, reason in problem_files:
            print(f"   {file_path.relative_to(directory_path)} - {reason}")
        
        print("\n💡 建议:")
        print("1. 安装Pillow库: pip install Pillow")
        print("2. 运行修复脚本: python scripts/fix_png_srgb.py")
        print("3. 或者忽略这些警告（已在代码中禁用）")
    else:
        print("✅ 所有PNG文件都没有sRGB配置文件问题")

def main():
    """主函数"""
    print("🔍 PNG图片sRGB配置文件检查工具")
    print("=" * 70)
    
    # 项目根目录
    project_root = Path(__file__).parent.parent
    
    # 检查resources/images目录
    images_dir = project_root / "resources" / "images"
    if images_dir.exists():
        print(f"📂 检查目录: {images_dir}")
        scan_png_files(images_dir)
    
    # 检查resources/qrc/images目录（如果存在）
    qrc_images_dir = project_root / "resources" / "qrc" / "images"
    if qrc_images_dir.exists():
        print(f"\n📂 检查目录: {qrc_images_dir}")
        scan_png_files(qrc_images_dir)

if __name__ == "__main__":
    main() 