#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
更新QRC文件路径的脚本
将items/路径更新为resources/images/路径
"""

import os
import re

def update_qrc_file(file_path):
    """更新单个QRC文件的路径"""
    print(f"更新文件: {file_path}")
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 替换路径：items/ -> resources/images/
    updated_content = content.replace('items/', 'resources/images/')
    
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(updated_content)
    
    print(f"✅ 已更新: {file_path}")

def main():
    """主函数"""
    qrc_dir = os.path.join('..', 'resources', 'qrc')  # 脚本在scripts目录下
    
    if not os.path.exists(qrc_dir):
        print(f"错误: QRC目录不存在: {qrc_dir}")
        return
    
    print("开始更新QRC文件路径...")
    print("="*50)
    
    # 遍历所有qrc文件
    for filename in os.listdir(qrc_dir):
        if filename.endswith('.qrc'):
            file_path = os.path.join(qrc_dir, filename)
            update_qrc_file(file_path)
    
    print("="*50)
    print("✅ 所有QRC文件路径更新完成!")

if __name__ == "__main__":
    main() 