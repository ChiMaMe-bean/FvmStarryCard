#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复QRC文件路径的脚本
将resources/images/路径修正为../images/路径（相对于qrc目录）
"""

import os
import re

def fix_qrc_file(file_path):
    """修复单个QRC文件的路径"""
    print(f"修复文件: {file_path}")
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 计算替换次数
    count_before = content.count('resources/images/')
    
    # 替换路径：resources/images/ -> ../images/
    # 因为QRC文件现在在resources/qrc/目录下，需要相对路径
    updated_content = content.replace('resources/images/', '../images/')
    
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(updated_content)
    
    if count_before > 0:
        print(f"✅ 已修复: {file_path} (修正了 {count_before} 个路径)")
    else:
        print(f"⭕ 跳过: {file_path} (无需修复)")

def main():
    """主函数"""
    qrc_dir = os.path.join('..', 'resources', 'qrc')  # 脚本在scripts目录下
    
    if not os.path.exists(qrc_dir):
        print(f"错误: QRC目录不存在: {qrc_dir}")
        return
    
    print("开始修复QRC文件路径...")
    print("="*50)
    
    # 遍历所有qrc文件
    qrc_files = []
    for filename in os.listdir(qrc_dir):
        if filename.endswith('.qrc'):
            qrc_files.append(os.path.join(qrc_dir, filename))
    
    if not qrc_files:
        print("未找到任何QRC文件")
        return
    
    print(f"找到 {len(qrc_files)} 个QRC文件")
    print("-"*50)
    
    # 修复每个文件
    for file_path in qrc_files:
        fix_qrc_file(file_path)
    
    print("="*50)
    print("✅ 所有QRC文件路径修复完成!")
    print("\n说明:")
    print("- QRC文件位于: resources/qrc/")
    print("- 图像文件位于: resources/images/")
    print("- 使用相对路径: ../images/ (从qrc目录到images目录)")

if __name__ == "__main__":
    main() 