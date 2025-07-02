@echo off
echo 🔄 更新资源文件...

:: 运行Python脚本生成QRC文件
python generate_qrc.py

if %errorlevel% neq 0 (
    echo ❌ 更新失败，请检查Python环境和脚本
    pause
    exit /b 1
)

echo ✅ 资源文件更新完成！
echo 💡 提示：如果添加了新的资源类别，请记得在CMakeLists.txt中添加对应的QRC文件
pause 