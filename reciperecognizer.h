#ifndef RECIPERECOGNIZER_H
#define RECIPERECOGNIZER_H
#include <QImage>
#include <QVector>
#include <QPoint>
#include <QString>
#include <QList>
#include <QPair>

class RecipeRecognizer {
public:
    static constexpr int RECIPE_AREA_X = 555;
    static constexpr int RECIPE_AREA_Y = 88;
    static constexpr int RECIPE_AREA_WIDTH = 365;
    static constexpr int RECIPE_AREA_HEIGHT = 200;
    static constexpr int GRID_VERTICAL_START = 4;
    static constexpr int GRID_VERTICAL_STEP = 49;

    void getRecipeGridLines(const QImage& recipeArea, QVector<int>& xLines, QVector<int>& yLines);
    void debugGridLines(const QImage& source);
    void drawDebugGridLines(QImage& debugImage, int startY);
    // 其它配方识别相关方法声明（如recognizeRecipe、calculateRecipeHistogram、findBestMatchesInGrid、recognizeRecipeInGrid、recognizeRecipeWithPaging、getAvailableRecipeTypes等）
    static bool isGridLineColor(const QColor& color);
    static bool findFirstCompleteLine(const QImage& image, int& outY);
};
#endif // RECIPERECOGNIZER_H 