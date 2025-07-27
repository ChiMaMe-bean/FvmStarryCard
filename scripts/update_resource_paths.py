#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
更新源代码中资源路径的脚本
根据新的QRC prefix更新所有资源引用
"""

import os
import re

def update_resource_paths(file_path):
    """更新单个源文件中的资源路径"""
    print(f"处理文件: {file_path}")
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        
        # 定义路径映射：旧路径模式 -> 新路径模式
        path_mappings = {
            r':/resources/images/background/': r':/background/../images/background/',
            r':/resources/images/bind_state/': r':/bind_state/../images/bind_state/', 
            r':/resources/images/card/': r':/card/../images/card/',
            r':/resources/images/clover/': r':/clover/../images/clover/',
            r':/resources/images/gameImage/': r':/gameImage/../images/gameImage/',
            r':/resources/images/icons/': r':/icons/../images/icons/',
            r':/resources/images/level/': r':/level/../images/level/',
            r':/resources/images/position/': r':/position/../images/position/',
            r':/resources/images/recipe/': r':/recipe/../images/recipe/',
            r':/resources/images/spices/': r':/spices/../images/spices/',
            r':/resources/images/spicesShow/': r':/spicesShow/../images/spicesShow/'
        }
        
        changes_made = 0
        for old_pattern, new_pattern in path_mappings.items():
            count = content.count(old_pattern)
            if count > 0:
                content = content.replace(old_pattern, new_pattern)
                changes_made += count
                print(f"  替换 {old_pattern} -> {new_pattern} ({count}次)")
        
        if changes_made > 0:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"✅ 已更新: {changes_made} 个路径")
        else:
            print(f"⭕ 跳过: 无需更新")
            
    except Exception as e:
        print(f"❌ 处理失败: {e}")

def scan_source_files(directory):
    """扫描源文件"""
    source_files = []
    
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cpp', '.h', '.c', '.hpp')):
                source_files.append(os.path.join(root, file))
    
    return source_files

def main():
    """主函数"""
    project_root = os.path.dirname(os.path.abspath(__file__ + "/.."))
    src_dir = os.path.join(project_root, "src")
    
    print("🔄 更新源代码中的资源路径")
    print("=" * 60)
    
    # 获取所有源文件
    source_files = scan_source_files(src_dir)
    
    if not source_files:
        print("未找到任何源文件")
        return
    
    print(f"找到 {len(source_files)} 个源文件")
    print("-" * 60)
    
    # 处理每个源文件
    for file_path in source_files:
        update_resource_paths(file_path)
    
    print("=" * 60)
    print("✅ 资源路径更新完成!")
    print("\n📝 注意：")
    print("1. 请重新编译项目")
    print("2. 新路径格式: :/prefix/../images/type/file.ext")

if __name__ == "__main__":
    main() 