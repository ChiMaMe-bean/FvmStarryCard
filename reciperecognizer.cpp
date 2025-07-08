#include "reciperecognizer.h"
#include "starrycard.h"    // 包含 LogType 的完整定义
#include <QPainter>
#include <QPen>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <chrono>
#include <algorithm>

// 构造函数
RecipeRecognizer::RecipeRecognizer() : recipeTemplatesLoaded(false)
{
}

// 析构函数
RecipeRecognizer::~RecipeRecognizer()
{
}

// 设置回调函数
void RecipeRecognizer::setLogCallback(LogCallback callback)
{
    m_logCallback = callback;
}

void RecipeRecognizer::setClickCallback(ClickCallback callback)
{
    m_clickCallback = callback;
}

void RecipeRecognizer::setCaptureCallback(CaptureCallback callback)
{
    m_captureCallback = callback;
}

// 内部辅助方法
void RecipeRecognizer::addLog(const QString& message, LogType type)
{
    if (m_logCallback) {
        m_logCallback(message, type);
    }
}

// 重载版本，默认使用 Info 类型
void RecipeRecognizer::addLog(const QString& message)
{
    addLog(message, LogType::Info);
}

bool RecipeRecognizer::leftClickDPI(HWND hwnd, int x, int y)
{
    if (m_clickCallback) {
        return m_clickCallback(hwnd, x, y);
    }
    return false;
}

QImage RecipeRecognizer::captureGameWindow()
{
    if (m_captureCallback) {
        return m_captureCallback();
    }
    return QImage();
}

// 计算颜色直方图
QVector<double> RecipeRecognizer::calculateColorHistogram(const QImage& image, const QRect& roi)
{
    QImage targetImage = image;
    
    // 如果指定了ROI区域，则裁剪图像
    if (!roi.isNull() && roi.isValid()) {
        targetImage = image.copy(roi);
    }
    
    // 转换为HSV色彩空间，更适合颜色比较
    QImage hsvImage = targetImage.convertToFormat(QImage::Format_RGB32);
    
    // 创建HSV直方图：H(色相)16个bin，S(饱和度)12个bin，V(明度)8个bin
    // 总共16*12*8 = 1536个特征
    const int hBins = 16, sBins = 12, vBins = 8;
    QVector<double> histogram(hBins * sBins * vBins, 0.0);
    
    int totalPixels = 0;
    
    for (int y = 0; y < hsvImage.height(); ++y) {
        for (int x = 0; x < hsvImage.width(); ++x) {
            QRgb pixel = hsvImage.pixel(x, y);
            
            // 转换RGB到HSV
            QColor color(pixel);
            int h, s, v;
            color.getHsv(&h, &s, &v);
            
            // 将HSV值映射到bin索引
            int hBin = qBound(0, h * hBins / 360, hBins - 1);
            int sBin = qBound(0, s * sBins / 256, sBins - 1);
            int vBin = qBound(0, v * vBins / 256, vBins - 1);
            
            // 计算在直方图中的索引
            int index = hBin * (sBins * vBins) + sBin * vBins + vBin;
            histogram[index] += 1.0;
            totalPixels++;
        }
    }
    
    // 归一化直方图
    if (totalPixels > 0) {
        for (int i = 0; i < histogram.size(); ++i) {
            histogram[i] /= totalPixels;
        }
    }
    
    return histogram;
}

// 计算颜色直方图相似度
double RecipeRecognizer::calculateColorHistogramSimilarity(const QImage& image1, const QImage& image2, const QRect& roi)
{
    QVector<double> hist1 = calculateColorHistogram(image1, roi);
    QVector<double> hist2 = calculateColorHistogram(image2, roi);
    
    if (hist1.size() != hist2.size() || hist1.isEmpty()) {
        return 0.0;
    }
    
    // 使用相关系数计算相似度
    double sum1 = 0.0, sum2 = 0.0, sum1Sq = 0.0, sum2Sq = 0.0, pSum = 0.0;
    int n = hist1.size();
    
    for (int i = 0; i < n; ++i) {
        sum1 += hist1[i];
        sum2 += hist2[i];
        sum1Sq += hist1[i] * hist1[i];
        sum2Sq += hist2[i] * hist2[i];
        pSum += hist1[i] * hist2[i];
    }
    
    // 计算皮尔逊相关系数
    double num = pSum - (sum1 * sum2 / n);
    double den = sqrt((sum1Sq - sum1 * sum1 / n) * (sum2Sq - sum2 * sum2 / n));
    
    if (den == 0) return 0.0;
    
    double correlation = num / den;
    
    // 将相关系数转换为0-1之间的相似度
    return (correlation + 1.0) / 2.0;
}

// 计算配方直方图
QVector<double> RecipeRecognizer::calculateRecipeHistogram(const QImage& image)
{
    // 转换为HSV色彩空间，更适合颜色比较
    QImage hsvImage = image.convertToFormat(QImage::Format_RGB32);
    
    // 创建HSV直方图：H(色相)16个bin，S(饱和度)12个bin，V(明度)8个bin
    // 总共16*12*8 = 1536个特征
    const int hBins = 16, sBins = 12, vBins = 8;
    QVector<double> histogram(hBins * sBins * vBins, 0.0);
    
    int totalPixels = 0;
    
    for (int y = 0; y < hsvImage.height(); ++y) {
        for (int x = 0; x < hsvImage.width(); ++x) {
            QRgb pixel = hsvImage.pixel(x, y);
            
            // 转换RGB到HSV
            QColor color(pixel);
            int h, s, v;
            color.getHsv(&h, &s, &v);
            
            // 将HSV值映射到bin索引
            int hBin = qBound(0, h * hBins / 360, hBins - 1);
            int sBin = qBound(0, s * sBins / 256, sBins - 1);
            int vBin = qBound(0, v * vBins / 256, vBins - 1);
            
            // 计算在直方图中的索引
            int index = hBin * (sBins * vBins) + sBin * vBins + vBin;
            histogram[index] += 1.0;
            totalPixels++;
        }
    }
    
    // 归一化直方图
    if (totalPixels > 0) {
        for (int i = 0; i < histogram.size(); ++i) {
            histogram[i] /= totalPixels;
        }
    }
    
    return histogram;
}

// 加载配方模板
bool RecipeRecognizer::loadRecipeTemplates()
{
    recipeTemplateHistograms.clear();
    recipeTemplateImages.clear();
    
    // 从资源文件中动态读取配方模板
    QDir resourceDir(":/items/recipe");
    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    QStringList recipeFiles = resourceDir.entryList(nameFilters, QDir::Files);
    
    if (recipeFiles.isEmpty()) {
        qDebug() << "未找到任何配方模板文件";
        addLog("未找到任何配方模板文件", LogType::Error);
        return false;
    }
    
    qDebug() << "找到配方模板文件:" << recipeFiles;
    addLog(QString("找到 %1 个配方模板文件").arg(recipeFiles.size()), LogType::Info);
    
    for (const QString& recipeFile : recipeFiles) {
        // 提取配方类型（文件名，不包含扩展名）
        QString recipeType = QFileInfo(recipeFile).baseName();
        QString filePath = QString(":/items/recipe/%1").arg(recipeFile);
        
        QImage template_image(filePath);
        
        if (template_image.isNull()) {
            qDebug() << "无法加载配方模板:" << recipeType << "路径:" << filePath;
            addLog(QString("无法加载配方模板: %1").arg(recipeType), LogType::Warning);
            continue;
        }
        
        // 保存模板图像和计算颜色直方图
        recipeTemplateImages[recipeType] = template_image;
        QVector<double> histogram = calculateRecipeHistogram(template_image);
        recipeTemplateHistograms[recipeType] = histogram;
        
        qDebug() << "成功加载配方模板:" << recipeType << "颜色直方图特征数:" << histogram.size();
        addLog(QString("成功加载配方模板: %1").arg(recipeType), LogType::Info);
        
        // 保存模板图像用于调试
        QString debugDir = QCoreApplication::applicationDirPath() + "/debug_recipe";
        QDir().mkpath(debugDir);
        
        QString templatePath = QString("%1/template_%2.png").arg(debugDir).arg(recipeType);
        if (template_image.save(templatePath)) {
            qDebug() << "配方模板图像已保存:" << templatePath;
        }
    }
    
    recipeTemplatesLoaded = !recipeTemplateHistograms.isEmpty();
    if (recipeTemplatesLoaded) {
        qDebug() << "配方模板加载完成，总数:" << recipeTemplateHistograms.size();
        QStringList loadedTypes = recipeTemplateHistograms.keys();
        addLog(QString("成功加载 %1 个配方模板: %2").arg(recipeTemplateHistograms.size()).arg(loadedTypes.join(", ")), LogType::Success);
    } else {
        qDebug() << "配方模板加载失败，没有成功加载任何模板";
        addLog("配方模板加载失败", LogType::Error);
    }
    
    return recipeTemplatesLoaded;
}

// 获取可用的配方类型
QStringList RecipeRecognizer::getAvailableRecipeTypes() const
{
    if (!recipeTemplatesLoaded) {
        return QStringList();
    }
    
    // 返回所有已加载的配方类型，按名称排序
    QStringList types = recipeTemplateHistograms.keys();
    std::sort(types.begin(), types.end());
    return types;
}

// 识别配方
QPair<QString, double> RecipeRecognizer::recognizeRecipe(const QImage& recipeArea)
{
    if (!recipeTemplatesLoaded) {
        addLog("配方模板未加载，无法进行识别", LogType::Error);
        return qMakePair("", 0.0);
    }
    
    if (recipeArea.isNull()) {
        addLog("配方区域图像无效", LogType::Error);
        return qMakePair("", 0.0);
    }
    
    // 计算当前配方区域的颜色直方图
    QVector<double> currentHistogram = calculateRecipeHistogram(recipeArea);
    
    QString bestMatch = "";
    double bestSimilarity = 0.0;
    QString secondBestMatch = "";
    double secondBestSimilarity = 0.0;
    
    // 与所有模板进行比较
    for (auto it = recipeTemplateHistograms.begin(); it != recipeTemplateHistograms.end(); ++it) {
        const QString& recipeType = it.key();
        const QVector<double>& templateHistogram = it.value();
        
        // 计算相似度
        double similarity = calculateColorHistogramSimilarity(recipeArea, recipeTemplateImages[recipeType]);
        
        qDebug() << "配方与" << recipeType << "的相似度:" << QString::number(similarity, 'f', 4);
        
        // 更新最佳匹配和次佳匹配
        if (similarity > bestSimilarity) {
            secondBestSimilarity = bestSimilarity;
            secondBestMatch = bestMatch;
            bestSimilarity = similarity;
            bestMatch = recipeType;
        } else if (similarity > secondBestSimilarity) {
            secondBestSimilarity = similarity;
            secondBestMatch = recipeType;
        }
    }
    
    // 输出调试信息
    qDebug() << "最佳匹配配方:" << bestMatch << "相似度:" << QString::number(bestSimilarity, 'f', 4);
    qDebug() << "次佳匹配配方:" << secondBestMatch << "相似度:" << QString::number(secondBestSimilarity, 'f', 4);
    
    addLog(QString("最佳匹配配方: %1 (相似度: %2)").arg(bestMatch).arg(QString::number(bestSimilarity, 'f', 4)), LogType::Info);
    addLog(QString("次佳匹配配方: %1 (相似度: %2)").arg(secondBestMatch).arg(QString::number(secondBestSimilarity, 'f', 4)), LogType::Info);
    
    return qMakePair(bestMatch, bestSimilarity);
}

// 在网格中找最佳匹配
QList<QPair<QPoint, double>> RecipeRecognizer::findBestMatchesInGrid(const QImage& recipeArea, const QString& targetRecipe)
{
    QList<QPair<QPoint, double>> matches;
    
    if (!recipeTemplatesLoaded || !recipeTemplateImages.contains(targetRecipe)) {
        addLog(QString("配方模板 %1 未加载").arg(targetRecipe), LogType::Error);
        return matches;
    }
    
    if (recipeArea.isNull()) {
        addLog("配方区域图像无效", LogType::Error);
        return matches;
    }
    
    QImage targetTemplate = recipeTemplateImages[targetRecipe];
    
    // 根据49x49的网格分割配方区域
    const int gridSize = 49;
    const int cols = recipeArea.width() / gridSize;  // 365 / 49 ≈ 7列
    const int rows = recipeArea.height() / gridSize; // 200 / 49 ≈ 4行
    
    addLog(QString("网格分割: %1行 x %2列").arg(rows).arg(cols), LogType::Info);
    
    // 遍历每个网格单元
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            // 提取当前网格单元
            QRect gridRect(col * gridSize, row * gridSize, gridSize, gridSize);
            QImage gridImage = recipeArea.copy(gridRect);
            
            if (gridImage.isNull()) {
                continue;
            }
            
            // 计算与目标模板的相似度
            double similarity = calculateColorHistogramSimilarity(gridImage, targetTemplate);
            
            // 记录网格位置和相似度
            QPoint gridPos(col, row);
            matches.append(qMakePair(gridPos, similarity));
            
            qDebug() << QString("网格(%1,%2) 与模板%3的相似度: %4")
                        .arg(col).arg(row).arg(targetRecipe).arg(QString::number(similarity, 'f', 4));
        }
    }
    
    // 按相似度排序，找出最佳匹配
    std::sort(matches.begin(), matches.end(), 
              [](const QPair<QPoint, double>& a, const QPair<QPoint, double>& b) {
                  return a.second > b.second;
              });
    
    // 输出前两个最佳匹配
    if (matches.size() >= 1) {
        QPoint bestPos = matches[0].first;
        double bestSimilarity = matches[0].second;
        addLog(QString("最佳匹配: 网格(%1,%2) 相似度: %3")
               .arg(bestPos.x()).arg(bestPos.y()).arg(QString::number(bestSimilarity, 'f', 4)), LogType::Success);
    }
    
    if (matches.size() >= 2) {
        QPoint secondPos = matches[1].first;
        double secondSimilarity = matches[1].second;
        addLog(QString("次佳匹配: 网格(%1,%2) 相似度: %3")
               .arg(secondPos.x()).arg(secondPos.y()).arg(QString::number(secondSimilarity, 'f', 4)), LogType::Success);
    }
    
    return matches;
}

// 在网格中识别配方
void RecipeRecognizer::recognizeRecipeInGrid(const QImage& screenshot, const QString& targetRecipe, HWND hwndGame)
{
    addLog("开始配方识别...", LogType::Info);
    
    // 提取配方区域（385x200）
    QImage recipeArea = screenshot.copy(555, 88, 365, 200);
    if (!recipeArea.isNull()) {
        // 保存配方区域图像用于调试
        QString appDir = QCoreApplication::applicationDirPath();
        QString debugDir = appDir + "/debug_recipe";
        QDir dir(debugDir);
        if (!dir.exists()) {
            dir.mkpath(debugDir);
        }
        
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
        QString recipeImagePath = debugDir + "/recipe_area_" + timestamp + ".png";
        if (recipeArea.save(recipeImagePath)) {
            addLog(QString("配方区域图像已保存: %1").arg(recipeImagePath), LogType::Info);
        }
        
        addLog(QString("选择匹配模板: %1").arg(targetRecipe), LogType::Info);
        
        // 获取分割线坐标
        QVector<int> xLines, yLines;
        getRecipeGridLines(recipeArea, xLines, yLines);
        addLog(QString("检测到横线y: %1, 纵线x: %2").arg(yLines.size()).arg(xLines.size()), LogType::Info);
        
        // 执行配方识别
        auto startTime = std::chrono::high_resolution_clock::now();
        QList<QPair<QPoint, double>> matches;
        QImage targetTemplate = recipeTemplateImages[targetRecipe];
        
        // 定义配方识别的ROI区域：配方坐标(4,4)开始的宽度为38，高度24的区域
        QRect recipeROI(4, 4, 38, 24);
        
        // 遍历所有格子
        for (int row = 0; row + 1 < yLines.size(); ++row) {
            for (int col = 0; col + 1 < xLines.size(); ++col) {
                int x0 = xLines[col];
                int y0 = yLines[row];
                int w = xLines[col + 1] - x0;
                int h = yLines[row + 1] - y0;
                if (w <= 0 || h <= 0) continue;
                QRect gridRect(x0, y0, w, h);
                QImage gridImage = recipeArea.copy(gridRect);
                if (gridImage.isNull()) continue;
                
                // 只使用配方ROI区域进行匹配
                QImage gridROI = gridImage.copy(recipeROI);
                if (gridROI.isNull()) continue;
                
                // 从模板中也提取对应的ROI区域进行比较
                QImage templateROI = targetTemplate.copy(recipeROI);
                if (templateROI.isNull()) continue;
                
                double similarity = calculateColorHistogramSimilarity(gridROI, templateROI);
                matches.append(qMakePair(QPoint(x0, y0), similarity));
            }
        }
        // 按相似度排序
        std::sort(matches.begin(), matches.end(), [](const QPair<QPoint, double>& a, const QPair<QPoint, double>& b) { return a.second > b.second; });
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        addLog(QString("配方识别耗时: %1 毫秒").arg(duration.count()), LogType::Info);
        
        // 保存最佳匹配的网格图像
        if (!matches.isEmpty()) {
            // 找到最佳和次佳匹配的格子索引
            auto findGridRect = [&](const QPoint& pos) -> QRect {
                int bestCol = -1, bestRow = -1;
                for (int col = 0; col + 1 < xLines.size(); ++col) {
                    if (xLines[col] == pos.x()) { bestCol = col; break; }
                }
                for (int row = 0; row + 1 < yLines.size(); ++row) {
                    if (yLines[row] == pos.y()) { bestRow = row; break; }
                }
                if (bestCol >= 0 && bestRow >= 0) {
                    int x0 = xLines[bestCol];
                    int y0 = yLines[bestRow];
                    int w = xLines[bestCol+1] - x0;
                    int h = yLines[bestRow+1] - y0;
                    return QRect(x0, y0, w, h);
                }
                return QRect();
            };
            QPoint bestPos = matches[0].first;
            double bestSim = matches[0].second;
            QRect bestGridRect = findGridRect(bestPos);
            if (bestGridRect.isValid()) {
                QImage bestGridImage = recipeArea.copy(bestGridRect);
                // 保存原始完整网格图片
                QString bestGridPath = debugDir + QString("/best_match_grid_%1_%2.png").arg(QString::number(bestSim, 'f', 4)).arg(timestamp);
                if (bestGridImage.save(bestGridPath)) {
                    addLog(QString("最佳匹配网格图像已保存: %1").arg(bestGridPath), LogType::Info);
                    qDebug() << "[DEBUG] 最佳匹配图片路径:" << bestGridPath << " 匹配度:" << bestSim << " 耗时:" << duration.count() << "ms";
                }
            }
            if (matches.size() >= 2) {
                QPoint secondPos = matches[1].first;
                double secondSim = matches[1].second;
                QRect secondGridRect = findGridRect(secondPos);
                if (secondGridRect.isValid()) {
                    QImage secondGridImage = recipeArea.copy(secondGridRect);
                    // 保存原始完整网格图片
                    QString secondGridPath = debugDir + QString("/second_match_grid_%1_%2.png").arg(QString::number(secondSim, 'f', 4)).arg(timestamp);
                    if (secondGridImage.save(secondGridPath)) {
                        addLog(QString("次佳匹配网格图像已保存: %1").arg(secondGridPath), LogType::Info);
                        qDebug() << "[DEBUG] 次佳匹配图片路径:" << secondGridPath << " 匹配度:" << secondSim << " 耗时:" << duration.count() << "ms";
                    }
                }
            }
        }
        // 检查是否有相似度为1的配方，如果有则点击其中心位置
        if (!matches.isEmpty() && matches[0].second >= 1.0) {
            QPoint bestPos = matches[0].first;
            double bestSim = matches[0].second;
            
            // 计算配方中心位置（相对于配方区域）
            int centerX = bestPos.x() + 24; // 网格中心x坐标
            int centerY = bestPos.y() + 24; // 网格中心y坐标
            
            // 转换为屏幕坐标（配方区域在屏幕上的位置是555,88）
            int screenX = 555 + centerX;
            int screenY = 88 + centerY;
            
            // 点击配方中心位置
            leftClickDPI(hwndGame, screenX, screenY);
            addLog(QString("点击配方中心位置: (%1, %2), 相似度: %3").arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4)), LogType::Success);
        }
    }
}

// 带翻页功能的配方识别
void RecipeRecognizer::recognizeRecipeWithPaging(const QImage& screenshot, const QString& targetRecipe, HWND hwndGame)
{
    addLog("开始带翻页功能的配方识别...", LogType::Info);
    // 配方区域参数
    const int recipeX = 555, recipeY = 88, recipeW = 365, recipeH = 200;
    const int recogH = 149; // 只识别上149像素
    QString appDir = QCoreApplication::applicationDirPath();
    QString debugDir = appDir + "/debug_recipe";
    QDir dir(debugDir); if (!dir.exists()) dir.mkpath(debugDir);

    auto recognizeOnePage = [&](const QImage& pageImg, const QString& timestamp) -> QList<QPair<QPoint, double>> {
        QImage recipeArea = pageImg.copy(recipeX, recipeY, recipeW, recipeH);
        QImage recognitionArea = recipeArea.copy(0, 0, recipeW, recogH);
        QVector<int> xLines, yLines;
        getRecipeGridLines(recognitionArea, xLines, yLines);
        auto startTime = std::chrono::high_resolution_clock::now();
        QList<QPair<QPoint, double>> matches;
        QImage targetTemplate = recipeTemplateImages[targetRecipe];
        QRect recipeROI(4, 4, 38, 24);
        for (int row = 0; row + 1 < yLines.size(); ++row) {
            for (int col = 0; col + 1 < xLines.size(); ++col) {
                int x0 = xLines[col];
                int y0 = yLines[row];
                int w = xLines[col + 1] - x0;
                int h = yLines[row + 1] - y0;
                if (w <= 0 || h <= 0) continue;
                QRect gridRect(x0, y0, w, h);
                QImage gridImage = recognitionArea.copy(gridRect);
                if (gridImage.isNull()) continue;
                QImage gridROI = gridImage.copy(recipeROI);
                if (gridROI.isNull()) continue;
                QImage templateROI = targetTemplate.copy(recipeROI);
                if (templateROI.isNull()) continue;
                double similarity = calculateColorHistogramSimilarity(gridROI, templateROI);
                matches.append(qMakePair(QPoint(x0, y0), similarity));
            }
        }
        std::sort(matches.begin(), matches.end(), [](const QPair<QPoint, double>& a, const QPair<QPoint, double>& b) { return a.second > b.second; });
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 保存最大和次大相似度图片
        auto findGridRect = [&](const QPoint& pos) -> QRect {
            int bestCol = -1, bestRow = -1;
            for (int col = 0; col + 1 < xLines.size(); ++col) if (xLines[col] == pos.x()) { bestCol = col; break; }
            for (int row = 0; row + 1 < yLines.size(); ++row) if (yLines[row] == pos.y()) { bestRow = row; break; }
            if (bestCol >= 0 && bestRow >= 0) {
                int x0 = xLines[bestCol], y0 = yLines[bestRow];
                int w = xLines[bestCol+1] - x0, h = yLines[bestRow+1] - y0;
                return QRect(x0, y0, w, h);
            }
            return QRect();
        };
        if (!matches.isEmpty()) {
            QPoint bestPos = matches[0].first; double bestSim = matches[0].second;
            QRect bestGridRect = findGridRect(bestPos);
            if (bestGridRect.isValid()) {
                QImage bestGridImage = recipeArea.copy(bestGridRect);
                QString bestGridPath = debugDir + QString("/best_match_grid_%1_%2.png").arg(QString::number(bestSim, 'f', 4)).arg(timestamp);
                if (bestGridImage.save(bestGridPath)) {
                    addLog(QString("最佳匹配网格图像已保存: %1").arg(bestGridPath), LogType::Info);
                    qDebug() << "[DEBUG] 最佳匹配图片路径:" << bestGridPath << " 匹配度:" << bestSim << " 耗时:" << duration.count() << "ms";
                }
            }
            if (matches.size() >= 2) {
                QPoint secondPos = matches[1].first; double secondSim = matches[1].second;
                QRect secondGridRect = findGridRect(secondPos);
                if (secondGridRect.isValid()) {
                    QImage secondGridImage = recipeArea.copy(secondGridRect);
                    QString secondGridPath = debugDir + QString("/second_match_grid_%1_%2.png").arg(QString::number(secondSim, 'f', 4)).arg(timestamp);
                    if (secondGridImage.save(secondGridPath)) {
                        addLog(QString("次佳匹配网格图像已保存: %1").arg(secondGridPath), LogType::Info);
                        qDebug() << "[DEBUG] 次佳匹配图片路径:" << secondGridPath << " 匹配度:" << secondSim << " 耗时:" << duration.count() << "ms";
                    }
                }
            }
        }
        return matches;
    };

    // 步骤1: 先识别当前页面的配方
    addLog("先识别当前页面的配方...", LogType::Info);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QList<QPair<QPoint, double>> currentMatches = recognizeOnePage(screenshot, timestamp);
    
    // 检查当前页面是否有相似度为1的配方在149区域内
    if (!currentMatches.isEmpty() && currentMatches[0].second >= 1.0) {
        QPoint bestPos = currentMatches[0].first;
        if (bestPos.y() < recogH) {
            double bestSim = currentMatches[0].second;
            int centerX = bestPos.x() + 24, centerY = bestPos.y() + 24;
            int screenX = recipeX + centerX, screenY = recipeY + centerY;
            leftClickDPI(hwndGame, screenX, screenY);
            addLog(QString("在当前页面找到配方并点击: (%1, %2), 相似度: %3").arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4)), LogType::Success);
            return;
        }
    }
    
    // 步骤2: 当前页面没找到，翻到顶部重新识别
    addLog("当前页面未找到配方，翻到顶部重新识别...", LogType::Info);
    // 翻到顶
    addLog("翻到顶...", LogType::Info);
    // 修正：点击配方区域内的(355, 20)（相对于全屏为(555+355, 88+20)）
    int clickTopX = recipeX + 355;
    int clickTopY = recipeY + 20;
    leftClickDPI(hwndGame, clickTopX, clickTopY);
    addLog(QString("[翻页] 点击配方区域顶部: 全屏坐标(%1, %2)").arg(clickTopX).arg(clickTopY), LogType::Info);
    qDebug() << "[翻页] 点击配方区域顶部: 全屏坐标(" << clickTopX << "," << clickTopY << ")";
    QThread::msleep(500);

    // 翻页到顶部后，重新截取游戏窗口
    QImage topScreenshot = captureGameWindow();
    if (topScreenshot.isNull()) {
        addLog("翻页到顶部后截图失败", LogType::Error);
        return;
    }
    addLog("翻页到顶部后重新截取游戏窗口", LogType::Info);

    QString timestamp2 = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    QList<QPair<QPoint, double>> matches = recognizeOnePage(topScreenshot, timestamp2);
    // 检查是否有相似度为1的配方在149区域内
    bool foundRecipe = false;
    if (!matches.isEmpty() && matches[0].second >= 1.0) {
        QPoint bestPos = matches[0].first;
        if (bestPos.y() < recogH) {
            foundRecipe = true;
            double bestSim = matches[0].second;
            int centerX = bestPos.x() + 24, centerY = bestPos.y() + 24;
            int screenX = recipeX + centerX, screenY = recipeY + centerY;
            leftClickDPI(hwndGame, screenX, screenY);
            addLog(QString("找到配方并点击: (%1, %2), 相似度: %3").arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4)), LogType::Success);
            return;
        }
    }
    
    // 翻页查找
    if (!foundRecipe) {
        addLog("开始翻页查找配方...", LogType::Info);
        int pageCount = 0, maxPages = 10;
        while (pageCount < maxPages) {
            // 修正：点击配方区域内的(355, 190)（相对于全屏为(555+355, 88+190)）
            int clickPageX = recipeX + 355;
            int clickPageY = recipeY + 190;
            leftClickDPI(hwndGame, clickPageX, clickPageY);
            addLog(QString("[翻页] 点击配方区域底部: 全屏坐标(%1, %2)").arg(clickPageX).arg(clickPageY), LogType::Info);
            qDebug() << "[翻页] 点击配方区域底部: 全屏坐标(" << clickPageX << "," << clickPageY << ")";
            QThread::msleep(500);
            pageCount++;
            addLog(QString("翻到第 %1 页").arg(pageCount), LogType::Info);
            QImage newScreenshot = captureGameWindow();
            if (newScreenshot.isNull()) { addLog("截图失败", LogType::Error); break; }
            QString timestamp3 = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
            QList<QPair<QPoint, double>> newMatches = recognizeOnePage(newScreenshot, timestamp3);
            if (!newMatches.isEmpty() && newMatches[0].second >= 1.0) {
                QPoint bestPos = newMatches[0].first;
                if (bestPos.y() < recogH) {
                    double bestSim = newMatches[0].second;
                    int centerX = bestPos.x() + 24, centerY = bestPos.y() + 24;
                    int screenX = recipeX + centerX, screenY = recipeY + centerY;
                    leftClickDPI(hwndGame, screenX, screenY);
                    addLog(QString("在第 %1 页找到配方并点击: (%2, %3), 相似度: %4").arg(pageCount).arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4)), LogType::Success);
                    return;
                }
            }
        }
        addLog("翻页完成，未找到目标配方", LogType::Warning);
    }
}

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
