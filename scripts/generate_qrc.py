#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
自动生成Qt资源文件(.qrc)的脚本
使用方法: python scripts/generate_qrc.py
支持新的文件系统结构: resources/qrc/images/
"""
import os
import sys
import re
import xml.etree.ElementTree as ET
from xml.dom import minidom
from pathlib import Path

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

def scan_resources(base_dir="resources/qrc/images", extensions=None):
    """扫描指定目录下的资源文件"""
    if extensions is None:
        extensions = {'.png', '.jpg', '.jpeg', '.svg', '.ico', '.gif', '.bmp'}
    
    # 获取项目根目录
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    full_base_dir = project_root / base_dir
    
    if not full_base_dir.exists():
        print_message(f"资源目录不存在: {full_base_dir}", 'error')
        return []
    
    resources = []
    for root, dirs, files in os.walk(full_base_dir):
        for file in files:
            if any(file.lower().endswith(ext) for ext in extensions):
                # 获取相对于images目录的路径
                file_path = Path(root) / file
                # 计算相对于images目录的路径
                relative_path = file_path.relative_to(full_base_dir)
                # 转换为QRC格式的路径: images/category/file.png
                qrc_path = f"images/{relative_path.as_posix()}"
                resources.append(qrc_path)
    
    # 使用自然排序
    return sorted(resources, key=natural_sort_key)

def create_qrc_by_category(resources, output_dir="resources/qrc", prefix="/"):
    """按类别创建多个QRC文件"""
    categories = {}
    
    # 按目录分类
    for resource in resources:
        parts = resource.split('/')
        if len(parts) >= 3:
            category = parts[1]  # images/category/file.png -> category
            if category not in categories:
                categories[category] = []
            categories[category].append(resource)
    
    # 获取项目根目录
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    full_output_dir = project_root / output_dir
    
    # 确保输出目录存在
    full_output_dir.mkdir(parents=True, exist_ok=True)
    
    qrc_files = []
    
    for category, files in categories.items():
        qrc_filename = f"resources_{category}.qrc"
        qrc_path = full_output_dir / qrc_filename
        
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
        
        # 移除空行和XML声明重复
        lines = pretty_xml.split('\n')
        filtered_lines = []
        xml_declaration_added = False
        
        for line in lines:
            stripped_line = line.strip()
            if not stripped_line:
                continue
            if stripped_line.startswith('<?xml') and xml_declaration_added:
                continue
            if stripped_line.startswith('<?xml'):
                xml_declaration_added = True
                filtered_lines.append("<?xml version='1.0' encoding='utf-8'?>")
            else:
                filtered_lines.append(line.rstrip())
        
        pretty_xml = '\n'.join(filtered_lines)
        
        with open(qrc_path, 'w', encoding='utf-8') as f:
            f.write(pretty_xml)
        
        qrc_files.append(qrc_filename)
        print_message(f"生成 {qrc_filename}: {len(files)} 个文件", 'success')
    
    return qrc_files

def create_single_qrc(resources, filename="resources.qrc", output_dir="resources/qrc", prefix="/"):
    """创建单个QRC文件"""
    # 获取项目根目录
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    full_output_dir = project_root / output_dir
    
    # 确保输出目录存在
    full_output_dir.mkdir(parents=True, exist_ok=True)
    
    qrc_path = full_output_dir / filename
    
    root = ET.Element("RCC", version="1.0")
    qresource = ET.SubElement(root, "qresource", prefix=prefix)
    
    for file_path in resources:
        file_elem = ET.SubElement(qresource, "file")
        file_elem.text = file_path
    
    # 格式化输出
    rough_string = ET.tostring(root, 'unicode')
    reparsed = minidom.parseString(rough_string)
    pretty_xml = reparsed.toprettyxml(indent="    ")
    
    # 移除空行和格式化XML声明
    lines = pretty_xml.split('\n')
    filtered_lines = []
    xml_declaration_added = False
    
    for line in lines:
        stripped_line = line.strip()
        if not stripped_line:
            continue
        if stripped_line.startswith('<?xml') and xml_declaration_added:
            continue
        if stripped_line.startswith('<?xml'):
            xml_declaration_added = True
            filtered_lines.append("<?xml version='1.0' encoding='utf-8'?>")
        else:
            filtered_lines.append(line.rstrip())
    
    pretty_xml = '\n'.join(filtered_lines)
    
    with open(qrc_path, 'w', encoding='utf-8') as f:
        f.write(pretty_xml)
    
    print_message(f"生成 {filename}: {len(resources)} 个文件", 'success')

def update_cmake_resources(qrc_files):
    """更新CMakeLists.txt中的资源文件列表"""
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    cmake_file = project_root / "CMakeLists.txt"
    
    if not cmake_file.exists():
        print_message("未找到CMakeLists.txt文件", 'error')
        return
    
    # 构建新的资源文件列表，使用相对路径
    qrc_list = '\n        '.join([f"resources/qrc/{qrc}" for qrc in qrc_files])
    
    print_message("请手动更新CMakeLists.txt中的资源文件列表:", 'note')
    print("将以下内容替换到RESOURCE_FILES变量中:")
    print(f"        {qrc_list}")

def show_current_structure():
    """显示当前的资源目录结构"""
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    images_dir = project_root / "resources" / "qrc" / "images"
    
    if not images_dir.exists():
        print_message(f"资源目录不存在: {images_dir}", 'error')
        return
    
    print_message("当前资源目录结构:", 'info')
    for item in sorted(images_dir.iterdir()):
        if item.is_dir():
            file_count = len([f for f in item.rglob('*') if f.is_file()])
            print(f"  📁 {item.name}/ ({file_count} 个文件)")

if __name__ == "__main__":
    try:
        print_message("QRC资源文件生成器", 'info')
        print_message("适用于新的文件系统结构: resources/qrc/images/", 'note')
        print("=" * 60)
        
        # 显示当前结构
        show_current_structure()
        print()
        
        print_message("扫描资源文件...", 'scan')
        resources = scan_resources()
        
        if not resources:
            print_message("未找到任何资源文件", 'error')
            sys.exit(1)
        
        print_message(f"找到 {len(resources)} 个资源文件", 'info')
        
        print("\n")
        print_message("请选择生成方式:", 'input')
        print("1. 按类别生成多个QRC文件 (推荐)")
        print("2. 生成单个QRC文件")
        print("3. 仅显示找到的资源文件")
        
        choice = input("\n请输入选择 (1/2/3): ").strip()
        
        if choice == "1":
            print_message("按类别生成QRC文件...", 'info')
            qrc_files = create_qrc_by_category(resources)
            update_cmake_resources(qrc_files)
        elif choice == "2":
            print_message("生成单个QRC文件...", 'info')
            create_single_qrc(resources, "resources_all.qrc")
            print_message("生成了 resources_all.qrc 文件", 'success')
        elif choice == "3":
            print_message("找到的资源文件列表:", 'info')
            for resource in resources:
                print(f"  {resource}")
        else:
            print_message("无效选择", 'error')
            sys.exit(1)
        
        print_message("完成！", 'success')
        if choice in ["1", "2"]:
            print_message("生成的QRC文件位于: resources/qrc/", 'note')
            print_message("提示：如果添加了新的资源类别，请记得在CMakeLists.txt中添加对应的QRC文件", 'note')
            
    except Exception as e:
        print_message(f"脚本执行失败: {str(e)}", 'error')
        import traceback
        traceback.print_exc()
        sys.exit(1) 
    