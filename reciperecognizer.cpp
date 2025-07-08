#include "reciperecognizer.h"
#include <QPainter>
#include <QPen>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <chrono>

void RecipeRecognizer::drawDebugGridLines(QImage& debugImage, int startY) {
    QPainter painter(&debugImage);
    painter.setPen(QPen(Qt::red, 2));
    for (int y = startY; y < debugImage.height(); y += 49) {
        painter.drawLine(0, y, debugImage.width(), y);
    }
    int startX = debugImage.width() / 3;
    for (int x = startX; x < debugImage.width(); x += 49) {
        painter.drawLine(x, startY, x, debugImage.height());
    }
}

void RecipeRecognizer::debugGridLines(const QImage& source) {
    auto startTime = std::chrono::high_resolution_clock::now();
    QImage recipeArea = source.copy(RECIPE_AREA_X, RECIPE_AREA_Y, RECIPE_AREA_WIDTH, RECIPE_AREA_HEIGHT);
    QImage debugImage = recipeArea.copy();
    QPainter painter(&debugImage);
    painter.setPen(QPen(Qt::red, 2));
    QVector<int> y1B354A;
    for (int y = 0; y < debugImage.height(); ++y) {
        int consecutiveCount = 0;
        for (int x = 16; x <= 40; ++x) {
            QColor color = debugImage.pixelColor(x, y);
            int r = color.red(), g = color.green(), b = color.blue();
            if (qAbs(r - 27) <= 20 && qAbs(g - 53) <= 20 && qAbs(b - 74) <= 20) {
                consecutiveCount++;
            } else {
                consecutiveCount = 0;
            }
            if (consecutiveCount >= 25) {
                break;
            }
        }
        if (consecutiveCount >= 25) {
            y1B354A.append(y);
            qDebug() << "找到合格的#1B354A线，y坐标:" << y << "连续像素点数:" << consecutiveCount;
            for (int x = 16; x <= 40; ++x) {
                painter.drawPoint(x, y);
            }
        }
    }
    std::sort(y1B354A.begin(), y1B354A.end());
    y1B354A.erase(std::unique(y1B354A.begin(), y1B354A.end()), y1B354A.end());
    qDebug() << "检测到的合格#1B354A颜色y坐标:" << y1B354A;
    if (y1B354A.isEmpty()) {
        qDebug() << "未检测到合格的#1B354A颜色线";
        return;
    }
    int minY = y1B354A.first();
    qDebug() << "最小#1B354A的y坐标:" << minY;
    qDebug() << "minY - 2 =" << (minY - 2) << "是否等于48:" << (minY - 2 == 48);
    int startY;
    bool use002347AsStart = false;
    if (minY - 2 == 48) {
        startY = 2;
        use002347AsStart = true;
        qDebug() << "使用#002347线作为起点，y坐标:" << startY;
    } else {
        startY = minY;
        use002347AsStart = false;
        qDebug() << "使用#1B354A线作为起点，y坐标:" << startY;
    }
    painter.setPen(QPen(Qt::green, 2));
    for (int y : y1B354A) {
        painter.drawLine(0, y, debugImage.width(), y);
        qDebug() << "绘制#1B354A横网格线，y坐标:" << y;
    }
    if (use002347AsStart) {
        painter.drawLine(0, 2, debugImage.width(), 2);
        qDebug() << "绘制#002347横网格线，y坐标: 2";
    }
    painter.setPen(QPen(Qt::blue, 2));
    for (int x = 4; x < debugImage.width(); x += 49) {
        painter.drawLine(x, 0, x, debugImage.height());
        qDebug() << "绘制纵网格线，x坐标:" << x;
    }
    if (use002347AsStart) {
        painter.setPen(QPen(Qt::blue, 3));
        for (int x = 16; x <= 40; ++x) {
            painter.drawPoint(x, startY);
        }
        qDebug() << "用蓝色标记#002347起点线，y坐标:" << startY;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    qDebug() << "网格线检测耗时:" << duration.count() << "毫秒";
    QString appDir = QCoreApplication::applicationDirPath();
    QString debugDir = appDir + "/data/debug_grid";
    QDir dir(debugDir);
    if (!dir.exists()) {
        dir.mkpath(debugDir);
    }
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QString debugFileName = debugDir + "/debug_grid_" + timestamp + ".png";
    if (debugImage.save(debugFileName)) {
        qDebug() << "调试图像已保存:" << debugFileName;
    } else {
        qDebug() << "保存调试图像失败";
    }
}

void RecipeRecognizer::getRecipeGridLines(const QImage& recipeArea, QVector<int>& xLines, QVector<int>& yLines) {
    // 横线y坐标：用#1B354A检测逻辑，必要时加#002347起点
    yLines.clear();
    for (int y = 0; y < recipeArea.height(); ++y) {
        int consecutiveCount = 0;
        for (int x = 16; x <= 40; ++x) {
            QColor color = recipeArea.pixelColor(x, y);
            int r = color.red(), g = color.green(), b = color.blue();
            if (qAbs(r - 27) <= 20 && qAbs(g - 53) <= 20 && qAbs(b - 74) <= 20) {
                consecutiveCount++;
            } else {
                consecutiveCount = 0;
            }
            if (consecutiveCount >= 25) {
                break;
            }
        }
        if (consecutiveCount >= 25) {
            yLines.append(y);
        }
    }
    std::sort(yLines.begin(), yLines.end());
    yLines.erase(std::unique(yLines.begin(), yLines.end()), yLines.end());
    if (!yLines.isEmpty()) {
        int minY = yLines.first();
        if (minY - 2 == 48) {
            yLines.prepend(2); // #002347线
        }
    }
    xLines.clear();
    for (int x = GRID_VERTICAL_START; x < recipeArea.width(); x += GRID_VERTICAL_STEP) {
        xLines.append(x);
    }
}

bool RecipeRecognizer::isGridLineColor(const QColor& color) {
    const int TOLERANCE = 20;
    if (qAbs(color.red() - 0) <= TOLERANCE && qAbs(color.green() - 35) <= TOLERANCE && qAbs(color.blue() - 71) <= TOLERANCE) return true;
    if (qAbs(color.red() - 20) <= TOLERANCE && qAbs(color.green() - 38) <= TOLERANCE && qAbs(color.blue() - 61) <= TOLERANCE) return true;
    if (qAbs(color.red() - 27) <= TOLERANCE && qAbs(color.green() - 53) <= TOLERANCE && qAbs(color.blue() - 74) <= TOLERANCE) return true;
    return false;
}

bool RecipeRecognizer::findFirstCompleteLine(const QImage& image, int& outY) {
    const int MIN_LINE_WIDTH = 5;
    const int START_X = image.width() / 3;
    for (int y = 0; y < image.height() - 49; y++) {
        int consecutivePoints = 0;
        for (int x = START_X; x < START_X + MIN_LINE_WIDTH; x++) {
            QColor pixelColor = image.pixelColor(x, y);
            if (isGridLineColor(pixelColor)) {
                consecutivePoints++;
            } else {
                break;
            }
        }
        if (consecutivePoints >= MIN_LINE_WIDTH) {
            QColor pixel49 = image.pixelColor(START_X, y + 49);
            if (isGridLineColor(pixel49)) {
                outY = y;
                return true;
            }
        }
    }
    return false;
}
