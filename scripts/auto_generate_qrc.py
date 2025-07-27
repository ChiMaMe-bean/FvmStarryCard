#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动生成分类QRC文件的简化脚本
直接运行: python auto_generate_qrc.py
"""
import os
import xml.etree.ElementTree as ET
from xml.dom import minidom

def create_qrc_file(category, files, prefix="/"):
    """创建单个QRC文件"""
    root = ET.Element("RCC", version="1.0")
    ET.SubElement(root, "qresource", prefix=prefix)
    qresource = root.find("qresource")
    
    for file_path in sorted(files):
        file_elem = ET.SubElement(qresource, "file")
        file_elem.text = file_path
    
    # 格式化输出
    rough_string = ET.tostring(root, 'unicode')
    reparsed = minidom.parseString(rough_string)
    pretty_xml = reparsed.toprettyxml(indent="    ")
    
    # 移除空行并添加XML声明
    lines = [line for line in pretty_xml.split('\n') if line.strip()]
    lines[0] = '<?xml version="1.0" encoding="UTF-8"?>'  # 替换XML声明
    lines.insert(1, '<!DOCTYPE RCC>')  # 添加DOCTYPE
    pretty_xml = '\n'.join(lines)
    
    filename = f"resources_{category}.qrc"
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(pretty_xml)
    
    print(f"✅ 生成 {filename}: {len(files)} 个文件")
    return filename

def scan_and_categorize():
    """扫描并分类资源文件"""
    extensions = {'.png', '.jpg', '.jpeg', '.svg', '.ico', '.gif', '.bmp'}
    categories = {}
    
    if not os.path.exists('items'):
        print("❌ 未找到items目录")
        return categories
    
    for root, dirs, files in os.walk('items'):
        for file in files:
            if any(file.lower().endswith(ext) for ext in extensions):
                file_path = os.path.join(root, file).replace('\\', '/')
                
                # 提取类别
                parts = file_path.split('/')
                if len(parts) >= 2:
                    category = parts[1]
                    if category not in categories:
                        categories[category] = []
                    categories[category].append(file_path)
    
    return categories

def main():
    print("🔍 扫描资源文件...")
    categories = scan_and_categorize()
    
    if not categories:
        print("❌ 没有找到资源文件")
        return
    
    qrc_files = []
    total_files = 0
    
    for category, files in categories.items():
        qrc_filename = create_qrc_file(category, files)
        qrc_files.append(qrc_filename)
        total_files += len(files)
    
    print(f"\n📊 统计:")
    print(f"   总共 {total_files} 个资源文件")
    print(f"   生成 {len(qrc_files)} 个QRC文件")
    
    print(f"\n📝 CMakeLists.txt中的资源配置:")
    print("   在PROJECT_SOURCES中使用以下QRC文件:")
    for qrc_file in qrc_files:
        print(f"        {qrc_file}")
    
    print("\n✅ 完成！")

if __name__ == "__main__":
    main() 