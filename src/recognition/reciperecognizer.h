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
#include <QElapsedTimer>
#include <QWindow>  // Qt中包含Windows类型定义
#include <chrono>



// 配方点击信息结构体
struct RecipeClickInfo {
    bool found = false;          // 是否找到配方
    QPoint clickPosition;        // 点击位置坐标
    double similarity = 0.0;     // 相似度
    
    RecipeClickInfo() = default;
    RecipeClickInfo(bool f, const QPoint& pos, double sim) 
        : found(f), clickPosition(pos), similarity(sim) {}
};

class RecipeRecognizer {
public:
    // 常量定义
    static constexpr int RECIPE_AREA_X = 555;
    static constexpr int RECIPE_AREA_Y = 88;
    static constexpr int RECIPE_AREA_WIDTH = 365;
    static constexpr int RECIPE_AREA_HEIGHT = 200;
    static constexpr int RECIPE_RECOGNITION_HEIGHT = 149; // 实际识别区域高度
    static constexpr int GRID_VERTICAL_START = 4;
    static constexpr int GRID_VERTICAL_STEP = 49;
    
    // 配方滚动条相关常量
    static constexpr int RECIPE_SCROLL_X = 910;  // 滚动条X坐标
    static constexpr int RECIPE_SCROLL_START_Y = 120; // 滚动条起始Y坐标
    static constexpr int RECIPE_GRID_HEIGHT = 49;  // 配方格子高度
    static constexpr int RECIPE_SCROLL_ROWS = 3;   // 每页显示3行完整配方（149/49≈3）
    // 计算精确翻页距离：确保每次滚动正好翻3行配方
    // 配方显示区域约149像素，每个配方格子49像素，所以每次滚动3*49=147像素

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
    
    // 图像哈希计算方法 (替代颜色直方图)
    QString calculateImageHash(const QImage& image, const QRect& roi = QRect());
    double calculateHashSimilarity(const QString& hash1, const QString& hash2);
    
    // 配方识别辅助方法
    QList<QPair<QPoint, double>> performGridHashMatching(const QImage& recipeArea, const QString& targetRecipe, 
                                                         const QVector<int>& xLines, const QVector<int>& yLines);
    void saveMatchDebugImages(const QList<QPair<QPoint, double>>& matches, const QImage& recipeArea,
                             const QVector<int>& xLines, const QVector<int>& yLines, 
                             const QString& debugDir, const QString& timestamp, int duration);
    
    // 常量定义
    static const QRect RECIPE_ROI; // 配方识别ROI区域
    
    // double calculateColorHistogramSimilarity(const QImage& image1, const QImage& image2, const QRect& roi = QRect());
    // QVector<double> calculateRecipeHistogram(const QImage& image); // 已弃用
    
    QPair<QString, double> recognizeRecipe(const QImage& recipeArea);
    QList<QPair<QPoint, double>> findBestMatchesInGrid(const QImage& recipeArea, const QString& targetRecipe);
    RecipeClickInfo recognizeRecipeInGrid(const QImage& screenshot, const QString& targetRecipe);
    RecipeClickInfo recognizeRecipeInCurrentPage(const QImage& screenshot, const QString& targetRecipe);
    
    // 动态识别方法 - 每10ms识别一次，匹配度<1时立即下一次，2秒超时
    RecipeClickInfo dynamicRecognizeRecipe(void* hwnd, const QString& windowName, const QString& targetRecipe);
    
    QStringList getAvailableRecipeTypes() const;

    // 设置DPI值
    void setDPI(int dpi) { DPI = dpi; }
    int getDPI() const { return DPI; }

    // 访问器方法
    bool isRecipeTemplatesLoaded() const { return recipeTemplatesLoaded; }
    const QMap<QString, QImage>& getRecipeTemplateImages() const { return recipeTemplateImages; }
    const QMap<QString, QString>& getRecipeTemplateHashes() const { return recipeTemplateHashes; }
    // const QHash<QString, QVector<double>>& getRecipeTemplateHistograms() const { return recipeTemplateHistograms; }

private:
    // 配方模板数据
    QMap<QString, QImage> recipeTemplateImages;        // 配方模板图像
    QMap<QString, QString> recipeTemplateHashes;       // 配方模板哈希值 (替代直方图)
    // QMap<QString, QVector<double>> recipeTemplateHistograms; // 配方模板直方图 (已弃用)
    bool recipeTemplatesLoaded;                        // 模板是否已加载
    
    // DPI设置
    int DPI;

    // 内部辅助方法
    QImage captureWindowByHandle(void* hwnd, const QString& windowName);
};

#endif // RECIPERECOGNIZER_H 
