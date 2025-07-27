#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试QRC文件路径有效性的脚本
"""

import os
import xml.etree.ElementTree as ET
from pathlib import Path

def test_qrc_file(qrc_path):
    """测试单个QRC文件中的所有路径"""
    print(f"\n📁 测试QRC文件: {qrc_path}")
    
    if not os.path.exists(qrc_path):
        print(f"❌ QRC文件不存在: {qrc_path}")
        return False
    
    try:
        tree = ET.parse(qrc_path)
        root = tree.getroot()
        
        # 获取QRC文件所在目录
        qrc_dir = os.path.dirname(qrc_path)
        
        success_count = 0
        total_count = 0
        
        for file_elem in root.findall('.//file'):
            file_path = file_elem.text
            total_count += 1
            
            # 计算相对于QRC文件的实际路径
            actual_path = os.path.join(qrc_dir, file_path)
            actual_path = os.path.normpath(actual_path)
            
            if os.path.exists(actual_path):
                print(f"✅ {file_path}")
                success_count += 1
            else:
                print(f"❌ {file_path} -> {actual_path}")
        
        print(f"📊 总计: {success_count}/{total_count} 成功")
        return success_count == total_count
        
    except Exception as e:
        print(f"❌ 解析QRC文件失败: {e}")
        return False

def main():
    """主函数"""
    project_root = os.path.dirname(os.path.abspath(__file__ + "/.."))
    qrc_dir = os.path.join(project_root, "resources", "qrc")
    
    print("🔍 测试QRC文件路径有效性")
    print("=" * 50)
    print(f"项目根目录: {project_root}")
    print(f"QRC目录: {qrc_dir}")
    
    if not os.path.exists(qrc_dir):
        print(f"❌ QRC目录不存在: {qrc_dir}")
        return
    
    # 获取所有QRC文件
    qrc_files = []
    for filename in os.listdir(qrc_dir):
        if filename.endswith('.qrc'):
            qrc_files.append(os.path.join(qrc_dir, filename))
    
    if not qrc_files:
        print("❌ 未找到任何QRC文件")
        return
    
    print(f"📄 找到 {len(qrc_files)} 个QRC文件")
    
    all_success = True
    for qrc_file in sorted(qrc_files):
        success = test_qrc_file(qrc_file)
        all_success = all_success and success
    
    print("\n" + "=" * 50)
    if all_success:
        print("🎉 所有QRC文件路径验证成功！")
    else:
        print("⚠️  发现路径问题，需要修复")
    
    # 额外检查：验证CMakeLists.txt中引用的QRC文件
    cmake_path = os.path.join(project_root, "CMakeLists.txt")
    if os.path.exists(cmake_path):
        print(f"\n🔧 检查CMakeLists.txt中的QRC引用...")
        with open(cmake_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        for qrc_file in qrc_files:
            rel_path = os.path.relpath(qrc_file, project_root)
            if rel_path.replace('\\', '/') in content:
                print(f"✅ {rel_path}")
            else:
                print(f"❌ CMakeLists.txt中未找到: {rel_path}")

if __name__ == "__main__":
    main() 