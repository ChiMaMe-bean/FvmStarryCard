#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复QRC文件路径的脚本
将相对路径改为简单文件名，并在编译时复制文件到QRC目录
"""

import os
import shutil
import xml.etree.ElementTree as ET

def copy_images_to_qrc_dir():
    """将images目录复制到qrc目录中"""
    project_root = os.path.dirname(os.path.abspath(__file__ + "/.."))
    source_images = os.path.join(project_root, "resources", "images")
    target_images = os.path.join(project_root, "resources", "qrc", "images")
    
    print(f"复制图片: {source_images} -> {target_images}")
    
    if os.path.exists(target_images):
        shutil.rmtree(target_images)
    
    shutil.copytree(source_images, target_images)
    print("✅ 图片复制完成")

def fix_qrc_simple_paths(qrc_path):
    """修复QRC文件，使用简单路径"""
    print(f"修复QRC文件: {qrc_path}")
    
    try:
        tree = ET.parse(qrc_path)
        root = tree.getroot()
        
        # 处理所有file元素
        for file_elem in root.findall('.//file'):
            old_path = file_elem.text
            if old_path and old_path.startswith('../images/'):
                # 将 ../images/level/1.png 改为 images/level/1.png
                new_path = old_path[3:]  # 移除 '../'
                file_elem.text = new_path
                print(f"  路径: {old_path} -> {new_path}")
        
        # 保存文件
        tree.write(qrc_path, encoding='utf-8', xml_declaration=True)
        print(f"✅ 修复完成: {qrc_path}")
        
    except Exception as e:
        print(f"❌ 修复失败: {e}")

def main():
    """主函数"""
    project_root = os.path.dirname(os.path.abspath(__file__ + "/.."))
    qrc_dir = os.path.join(project_root, "resources", "qrc")
    
    print("🔧 修复QRC文件路径问题")
    print("=" * 50)
    
    # 步骤1：复制图片到qrc目录
    copy_images_to_qrc_dir()
    
    print("\n" + "-" * 50)
    
    # 步骤2：修复所有QRC文件的路径
    for filename in os.listdir(qrc_dir):
        if filename.endswith('.qrc'):
            qrc_path = os.path.join(qrc_dir, filename)
            fix_qrc_simple_paths(qrc_path)
    
    print("=" * 50)
    print("✅ QRC路径修复完成!")
    print("\n📝 说明：")
    print("1. 图片已复制到 resources/qrc/images/")
    print("2. QRC文件使用 images/type/file.png 格式")
    print("3. 请重新编译项目")

if __name__ == "__main__":
    main() 