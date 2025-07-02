#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动生成Qt资源文件(.qrc)的脚本
使用方法: python generate_qrc.py
"""
import os
import sys
import re
import xml.etree.ElementTree as ET
from xml.dom import minidom

def print_message(message, type='info'):
    """打印带有图标的消息"""
    icons = {
        'info': '🔄',
        'error': '❌',
        'success': '✅',
        'note': '💡',
        'scan': '🔍',
        'input': '📝'
    }
    icon = icons.get(type, '')
    print(f"[{icon}] {message}")

def natural_sort_key(s):
    """实现自然排序的键函数"""
    # 将字符串分割成数字和非数字部分
    def convert(text):
        return int(text) if text.isdigit() else text.lower()
    return [convert(c) for c in re.split('([0-9]+)', s)]

# 设置标准输出编码为UTF-8
if sys.platform.startswith('win'):
    import codecs
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, 'strict')
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr.buffer, 'strict')

def scan_resources(base_dir="items", extensions=None):
    """扫描指定目录下的资源文件"""
    if extensions is None:
        extensions = {'.png', '.jpg', '.jpeg', '.svg', '.ico', '.gif', '.bmp'}
    
    resources = []
    for root, dirs, files in os.walk(base_dir):
        for file in files:
            if any(file.lower().endswith(ext) for ext in extensions):
                file_path = os.path.join(root, file).replace('\\', '/')
                resources.append(file_path)
    
    # 使用自然排序
    return sorted(resources, key=natural_sort_key)

def create_qrc_by_category(resources, output_dir=".", prefix="/"):
    """按类别创建多个QRC文件"""
    categories = {}
    
    # 按目录分类
    for resource in resources:
        parts = resource.split('/')
        if len(parts) >= 2:
            category = parts[1]  # items/category/file.png -> category
            if category not in categories:
                categories[category] = []
            categories[category].append(resource)
    
    qrc_files = []
    
    for category, files in categories.items():
        qrc_filename = f"resources_{category}.qrc"
        qrc_path = os.path.join(output_dir, qrc_filename)
        
        # 创建QRC文件
        root = ET.Element("RCC", version="1.0")
        qresource = ET.SubElement(root, "qresource", prefix=prefix)
        
        for file_path in files:
            file_elem = ET.SubElement(qresource, "file")
            file_elem.text = file_path
        
        # 格式化输出
        rough_string = ET.tostring(root, 'unicode')
        reparsed = minidom.parseString(rough_string)
        pretty_xml = reparsed.toprettyxml(indent="    ")
        
        # 移除空行
        lines = [line for line in pretty_xml.split('\n') if line.strip()]
        pretty_xml = '\n'.join(lines)
        
        with open(qrc_path, 'w', encoding='utf-8') as f:
            f.write(pretty_xml)
        
        qrc_files.append(qrc_filename)
        print(f"生成 {qrc_filename}: {len(files)} 个文件")
    
    return qrc_files

def create_single_qrc(resources, filename="resources.qrc", prefix="/"):
    """创建单个QRC文件"""
    root = ET.Element("RCC", version="1.0")
    qresource = ET.SubElement(root, "qresource", prefix=prefix)
    
    for file_path in resources:
        file_elem = ET.SubElement(qresource, "file")
        file_elem.text = file_path
    
    # 格式化输出
    rough_string = ET.tostring(root, 'unicode')
    reparsed = minidom.parseString(rough_string)
    pretty_xml = reparsed.toprettyxml(indent="    ")
    
    # 移除空行
    lines = [line for line in pretty_xml.split('\n') if line.strip()]
    pretty_xml = '\n'.join(lines)
    
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(pretty_xml)
    
    print(f"生成 {filename}: {len(resources)} 个文件")

def update_cmake_resources(qrc_files):
    """更新CMakeLists.txt中的资源文件列表"""
    cmake_file = "CMakeLists.txt"
    if not os.path.exists(cmake_file):
        print("未找到CMakeLists.txt文件")
        return
    
    with open(cmake_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 构建新的资源文件列表
    qrc_list = '\n        '.join(qrc_files)
    
    print("请手动更新CMakeLists.txt中的资源文件列表:")
    print("将以下内容替换到PROJECT_SOURCES中的resources.qrc:")
    print(f"        {qrc_list}")

if __name__ == "__main__":
    try:
        print_message("扫描资源文件...", 'scan')
        resources = scan_resources()
        print_message(f"找到 {len(resources)} 个资源文件", 'info')
        
        print("\n")
        print_message("请选择生成方式:", 'input')
        print("1. 按类别生成多个QRC文件 (推荐)")
        print("2. 生成单个QRC文件")
        
        choice = input("\n请输入选择 (1/2): ").strip()
        
        if choice == "1":
            qrc_files = create_qrc_by_category(resources)
            update_cmake_resources(qrc_files)
        else:
            create_single_qrc(resources, "resources_new.qrc")
            print_message("生成了 resources_new.qrc 文件", 'success')
        
        print_message("完成！", 'success')
        print_message("提示：如果添加了新的资源类别，请记得在CMakeLists.txt中添加对应的QRC文件", 'note')
    except Exception as e:
        print_message(f"更新失败，请检查Python环境和脚本: {str(e)}", 'error')
        sys.exit(1) 
    