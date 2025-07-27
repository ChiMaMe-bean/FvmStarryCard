#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
更新源代码中资源路径的脚本
将:/items/路径更新为:/resources/images/路径
"""

import os
import re

def update_source_file(file_path):
    """更新单个源文件的资源路径"""
    print(f"更新文件: {file_path}")
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"❌ 读取文件失败: {file_path}, 错误: {e}")
        return
    
    # 计算替换次数
    count_before = content.count(':/items/')
    
    # 替换路径：:/items/ -> :/resources/images/
    updated_content = content.replace(':/items/', ':/resources/images/')
    
    count_after = updated_content.count(':/resources/images/')
    
    if count_before > 0:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(updated_content)
        print(f"✅ 已更新: {file_path} (替换了 {count_before} 个路径)")
    else:
        print(f"⭕ 跳过: {file_path} (无需更新)")

def scan_directory(directory):
    """扫描目录中的所有源文件"""
    source_files = []
    
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cpp', '.h', '.c', '.hpp')):
                source_files.append(os.path.join(root, file))
    
    return source_files

def main():
    """主函数"""
    src_dir = os.path.join('..', 'src')  # 脚本在scripts目录下
    
    if not os.path.exists(src_dir):
        print(f"错误: 源代码目录不存在: {src_dir}")
        return
    
    print("开始更新源代码中的资源路径...")
    print("="*60)
    
    # 扫描所有源文件
    source_files = scan_directory(src_dir)
    
    if not source_files:
        print("未找到任何源文件")
        return
    
    print(f"找到 {len(source_files)} 个源文件")
    print("-"*60)
    
    # 更新每个文件
    for file_path in source_files:
        update_source_file(file_path)
    
    print("="*60)
    print("✅ 所有源文件资源路径更新完成!")

if __name__ == "__main__":
    main() 