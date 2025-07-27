#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
更新源代码资源路径的脚本
将复杂的prefix路径改为简单的QRC路径
"""

import os

def update_source_paths(file_path):
    """更新单个源文件中的资源路径"""
    print(f"处理文件: {file_path}")
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 定义路径映射：复杂路径 -> 简单路径
        path_mappings = {
            ':/background/../images/background/': ':/background/',
            ':/bind_state/../images/bind_state/': ':/bind_state/', 
            ':/card/../images/card/': ':/card/',
            ':/clover/../images/clover/': ':/clover/',
            ':/gameImage/../images/gameImage/': ':/gameImage/',
            ':/icons/../images/icons/': ':/icons/',
            ':/level/../images/level/': ':/level/',
            ':/position/../images/position/': ':/position/',
            ':/recipe/../images/recipe/': ':/recipe/',
            ':/spices/../images/spices/': ':/spices/',
            ':/spicesShow/../images/spicesShow/': ':/spicesShow/'
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
    
    print("🔄 更新源代码中的资源路径为简化格式")
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
        update_source_paths(file_path)
    
    print("=" * 60)
    print("✅ 资源路径更新完成!")
    print("\n📝 新格式说明：")
    print(":/background/default.png")
    print(":/level/1.png")
    print(":/icons/icon128.ico")

if __name__ == "__main__":
    main() 