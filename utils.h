#ifndef UTILS_H
#define UTILS_H

#include <QPixmap>
#include <QPalette>
#include <QWidget>
#include <QDebug>
#include <Windows.h>
#include <QPoint>
#include <QString>

class Utils {
public:
    static void setBackground(QWidget *widget, const QString &imagePath) {
        QPixmap pixmap(imagePath);
        if (!pixmap.isNull()) {
            QPalette palette = widget->palette();
            int windowWidth = widget->width();
            int windowHeight = widget->height();

            // 按比例缩放图片，使图片至少有一个维度与窗口尺寸一致
            QPixmap scaledPixmap = pixmap.scaled(
                windowWidth, windowHeight,
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation
            );

            // 计算裁剪区域，保证从图片中心裁剪
            int scaledWidth = scaledPixmap.width();
            int scaledHeight = scaledPixmap.height();
            int x = (scaledWidth - windowWidth) / 2;
            int y = (scaledHeight - windowHeight) / 2;

            // 裁剪图片
            QPixmap croppedPixmap = scaledPixmap.copy(x, y, windowWidth, windowHeight);

            palette.setBrush(QPalette::Window, QBrush(croppedPixmap));
            widget->setPalette(palette);
        } else {
            qDebug() << "图片加载失败，请检查路径";
        }
    }
};

class WindowUtils {
public:
    static int getDPI();
    static HWND getWindowFromPoint(const QPoint& point);
    static bool isGameWindow(HWND hwnd);
    static void clickAtPosition(HWND hwnd, int x, int y);
    static QString getWindowTitle(HWND hwnd);
    static QString getParentWindowTitle(HWND hwnd);
    static HWND getParentWindow(HWND hwnd);
    
private:
    static double getDPIScaleFactor();
    static void simulateMouseClick(int x, int y);
};

#endif // UTILS_H
