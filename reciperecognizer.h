#ifndef RECIPERECOGNIZER_H
#define RECIPERECOGNIZER_H

#include <QImage>
#include <QVector>
#include <QPoint>
#include <QString>
#include <QList>
#include <QPair>
#include <QHash>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QColor>
#include <QRect>
#include <QTime>
#include <QElapsedTimer>
#include <windows.h>
#include <functional>
#include <chrono>

// 前向声明 LogType 枚举（定义在 starrycard.h 中）
enum class LogType;

class RecipeRecognizer {
public:
    // 常量定义
    static constexpr int RECIPE_AREA_X = 555;
    static constexpr int RECIPE_AREA_Y = 88;
    static constexpr int RECIPE_AREA_WIDTH = 365;
    static constexpr int RECIPE_AREA_HEIGHT = 200;
    static constexpr int GRID_VERTICAL_START = 4;
    static constexpr int GRID_VERTICAL_STEP = 49;

    // 构造函数
    RecipeRecognizer();
    ~RecipeRecognizer();

    // 现有的网格线相关方法
    void getRecipeGridLines(const QImage& recipeArea, QVector<int>& xLines, QVector<int>& yLines);
    void debugGridLines(const QImage& source);
    void drawDebugGridLines(QImage& debugImage, int startY);
    static bool isGridLineColor(const QColor& color);
    static bool findFirstCompleteLine(const QImage& image, int& outY);

    // 新增的配方识别相关方法
    bool loadRecipeTemplates();
    QVector<double> calculateRecipeHistogram(const QImage& image);
    QVector<double> calculateColorHistogram(const QImage& image, const QRect& roi = QRect());
    double calculateColorHistogramSimilarity(const QImage& image1, const QImage& image2, const QRect& roi = QRect());
    QPair<QString, double> recognizeRecipe(const QImage& recipeArea);
    QList<QPair<QPoint, double>> findBestMatchesInGrid(const QImage& recipeArea, const QString& targetRecipe);
    void recognizeRecipeInGrid(const QImage& screenshot, const QString& targetRecipe, HWND hwndGame);
    void recognizeRecipeWithPaging(const QImage& screenshot, const QString& targetRecipe, HWND hwndGame);
    QStringList getAvailableRecipeTypes() const;

    // 设置DPI值
    void setDPI(int dpi) { DPI = dpi; }
    int getDPI() const { return DPI; }
    
    // 设置游戏窗口句柄（用于后续操作）
    void setGameWindow(HWND hwnd) { gameWindow = hwnd; }
    HWND getGameWindow() const { return gameWindow; }

    // 访问器方法
    bool isRecipeTemplatesLoaded() const { return recipeTemplatesLoaded; }
    const QHash<QString, QImage>& getRecipeTemplateImages() const { return recipeTemplateImages; }
    const QHash<QString, QVector<double>>& getRecipeTemplateHistograms() const { return recipeTemplateHistograms; }

private:
    // 配方模板数据
    QHash<QString, QVector<double>> recipeTemplateHistograms;
    QHash<QString, QImage> recipeTemplateImages;
    bool recipeTemplatesLoaded;
    
    // DPI 设置和游戏窗口句柄
    int DPI;
    HWND gameWindow;

    // 内部辅助方法（原来的回调函数现在变成直接实现）
    void addLog(const QString& message, LogType type);
    void addLog(const QString& message);  // 重载版本提供默认行为
    bool leftClickDPI(HWND hwnd, int x, int y);
    QImage captureWindowByHandle(HWND hwnd, const QString& windowName);
    void sleepByQElapsedTimer(int ms);
};

#endif // RECIPERECOGNIZER_H 