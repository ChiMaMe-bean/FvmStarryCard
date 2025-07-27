#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复QRC文件prefix的脚本
为每个QRC文件设置独特的prefix，避免资源命名空间冲突
"""

import os
import xml.etree.ElementTree as ET

def fix_qrc_prefix(qrc_path, new_prefix):
    """修复单个QRC文件的prefix"""
    print(f"修复文件: {qrc_path}")
    print(f"设置prefix: {new_prefix}")
    
    try:
        tree = ET.parse(qrc_path)
        root = tree.getroot()
        
        # 找到qresource元素并修改prefix
        qresource = root.find('qresource')
        if qresource is not None:
            old_prefix = qresource.get('prefix', '/')
            qresource.set('prefix', new_prefix)
            
            # 保存文件
            tree.write(qrc_path, encoding='utf-8', xml_declaration=True)
            print(f"✅ 已修复: {old_prefix} -> {new_prefix}")
        else:
            print(f"❌ 未找到qresource元素")
            
    except Exception as e:
        print(f"❌ 处理文件失败: {e}")

def main():
    """主函数"""
    project_root = os.path.dirname(os.path.abspath(__file__ + "/.."))
    qrc_dir = os.path.join(project_root, "resources", "qrc")
    
    print("🔧 修复QRC文件prefix设置")
    print("=" * 50)
    
    # 定义每个QRC文件的prefix映射
    prefix_mapping = {
        "resources_background.qrc": "/background",
        "resources_bind_state.qrc": "/bind_state", 
        "resources_card.qrc": "/card",
        "resources_clover.qrc": "/clover",
        "resources_gameImage.qrc": "/gameImage",
        "resources_icons.qrc": "/icons",
        "resources_level.qrc": "/level",
        "resources_position.qrc": "/position",
        "resources_recipe.qrc": "/recipe",
        "resources_spices.qrc": "/spices",
        "resources_spicesShow.qrc": "/spicesShow"
    }
    
    # 处理每个QRC文件
    for filename, prefix in prefix_mapping.items():
        qrc_path = os.path.join(qrc_dir, filename)
        if os.path.exists(qrc_path):
            fix_qrc_prefix(qrc_path, prefix)
        else:
            print(f"⚠️ 文件不存在: {qrc_path}")
    
    print("=" * 50)
    print("✅ QRC prefix修复完成!")
    print("\n📝 注意：修复后需要：")
    print("1. 更新源代码中的资源路径")
    print("2. 重新编译项目")

if __name__ == "__main__":
    main() 