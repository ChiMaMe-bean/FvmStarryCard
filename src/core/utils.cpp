#include "utils.h"
#include <QDebug>

int WindowUtils::getDPI()
{
    HWND desktop = GetDesktopWindow();
    HDC dc = GetDC(desktop);
    int dpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(desktop, dc);
    return dpi;
}

double WindowUtils::getDPIScaleFactor()
{
    return static_cast<double>(getDPI()) / 96.0;
}

HWND WindowUtils::getWindowFromPoint(const QPoint& point)
{
    // 获取DPI缩放因子
    double scaleFactor = getDPIScaleFactor();
    
    // 将Qt坐标转换为屏幕坐标
    POINT pt;
    pt.x = static_cast<LONG>(point.x() * scaleFactor);
    pt.y = static_cast<LONG>(point.y() * scaleFactor);

    // 获取点击位置的窗口句柄
    HWND hwnd = WindowFromPoint(pt);
    if (!hwnd) return nullptr;

    return hwnd;
}

bool WindowUtils::isGameWindow(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return false;
        
    wchar_t className[256];
    GetClassName(hwnd, className, 256);
    return wcscmp(className, L"NativeWindowClass") == 0 || 
           wcscmp(className, L"WebPluginView") == 0;
}

HWND WindowUtils::getParentWindow(HWND hwnd)
{
    if (!hwnd) return nullptr;
    return GetParent(hwnd);
}

QString WindowUtils::getParentWindowTitle(HWND hwnd)
{
    HWND parentHwnd = getParentWindow(hwnd);
    if (!parentHwnd) return getWindowTitle(hwnd);
    return getWindowTitle(parentHwnd);
}

QString WindowUtils::getWindowTitle(HWND hwnd)
{
    if (!hwnd)
        return QString();

    wchar_t title[256];
    GetWindowText(hwnd, title, 256);
    return QString::fromWCharArray(title);
}