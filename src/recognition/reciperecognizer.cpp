#include "reciperecognizer.h"
#include "../core/starrycard.h"    // 包含 LogType 的完整定义
#include <QPainter>
#include <QPen>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <chrono>
#include <algorithm>

// 构造函数
RecipeRecognizer::RecipeRecognizer() : recipeTemplatesLoaded(false), DPI(96), gameWindow(nullptr)
{
}

// 析构函数
RecipeRecognizer::~RecipeRecognizer()
{
}

// 内部辅助方法（原来的回调函数现在变成直接实现）
void RecipeRecognizer::addLog(const QString& message, LogType type)
{
    // 获取当前时间
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    
    // 根据日志类型设置前缀
    QString typePrefix;
    switch (type) {
        case LogType::Success:
            typePrefix = "[SUCCESS]";
            break;
        case LogType::Warning:
            typePrefix = "[WARNING]";
            break;
        case LogType::Error:
            typePrefix = "[ERROR]";
            break;
        case LogType::Info:
            typePrefix = "[INFO]";
            break;
        default:
            typePrefix = "[LOG]";
    }
    
    // 输出到调试控制台
    qDebug() << QString("%1 %2 %3").arg(timestamp, typePrefix, message);
}

// 重载版本，默认使用 Info 类型
void RecipeRecognizer::addLog(const QString& message)
{
    qDebug() << message;
}

bool RecipeRecognizer::leftClickDPI(HWND hwnd, int x, int y)
{
    double scaleFactor = static_cast<double>(DPI) / 96.0;
    
    // 计算DPI缩放后的坐标
    int scaledX = static_cast<int>(x * scaleFactor);
    int scaledY = static_cast<int>(y * scaleFactor);

    // 发送鼠标消息
    BOOL bResult = PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(scaledX, scaledY));
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(scaledX, scaledY));
    return bResult != 0;
}

QImage RecipeRecognizer::captureWindowByHandle(HWND hwnd, const QString& windowName)
{
    // 如果传入的句柄无效，尝试使用成员变量的句柄
    if (!hwnd || !IsWindow(hwnd)) {
        if (gameWindow && IsWindow(gameWindow)) {
            hwnd = gameWindow;
            qDebug() << QString("使用成员变量窗口句柄替代无效句柄: %1").arg(windowName);
        } else {
            qDebug() << QString("无效的窗口句柄且无备用句柄: %1").arg(windowName);
            return QImage();
        }
    }

    // 获取窗口位置和大小
    RECT rect;
    if(windowName == "主页面") // 主页面截图区域
    {
        rect.left = 0;
        rect.top = 0;
        rect.right = 950;
        rect.bottom = 596;
    }
    else if (!GetWindowRect(hwnd, &rect)) {
        qDebug() << QString("获取窗口位置失败: %1").arg(windowName);
        return QImage();
    }
    
    // 获取窗口DC
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) {
        qDebug() << QString("获取窗口DC失败: %1").arg(windowName);
        return QImage();
    }

    // 创建兼容DC和位图
    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        qDebug() << QString("创建兼容DC失败: %1").arg(windowName);
        ReleaseDC(hwnd, hdcWindow);
        return QImage();
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    if (!hBitmap) {
        qDebug() << QString("创建兼容位图失败: %1").arg(windowName);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return QImage();
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemDC, hBitmap);

    // 复制窗口内容到位图
    if (!BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY)) {
        qDebug() << QString("复制窗口内容失败: %1").arg(windowName);
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return QImage();
    }

    // 转换为QImage
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    
    // 设置BITMAPINFO结构
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // 负值表示自上而下的位图
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // 获取位图数据
    if (!GetDIBits(hdcMemDC, hBitmap, 0, height, image.bits(), &bmi, DIB_RGB_COLORS)) {
        qDebug() << QString("获取位图数据失败: %1").arg(windowName);
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return QImage();
    }

    // 清理资源
    SelectObject(hdcMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);

    qDebug() << QString("成功截取%1窗口图像：%2x%3").arg(windowName).arg(width).arg(height);
    return image;
}

void RecipeRecognizer::sleepByQElapsedTimer(int ms)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms)
    {
        QCoreApplication::processEvents(); // 不停地处理事件，让程序保持响应
    }
}

// 计算图像哈希值
QString RecipeRecognizer::calculateImageHash(const QImage& image, const QRect& roi)
{
    QImage targetImage = image;
    
    // 如果指定了ROI区域，则裁剪图像
    if (!roi.isNull() && roi.isValid()) {
        targetImage = image.copy(roi);
    }
    
    // 转换为灰度并缩放为8x8像素进行哈希计算
    QImage grayImage = targetImage.convertToFormat(QImage::Format_Grayscale8);
    QImage hashImage = grayImage.scaled(8, 8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    
    // 计算平均像素值
    qint64 totalValue = 0;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            totalValue += qGray(hashImage.pixel(x, y));
        }
    }
    qint64 avgValue = totalValue / 64;
    
    // 生成64位哈希值
    QString hash;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            int pixelValue = qGray(hashImage.pixel(x, y));
            hash += (pixelValue >= avgValue) ? "1" : "0";
        }
    }
    
    return hash;
}

// 计算哈希相似度 (基于汉明距离)
double RecipeRecognizer::calculateHashSimilarity(const QString& hash1, const QString& hash2)
{
    if (hash1.length() != hash2.length() || hash1.isEmpty()) {
        return 0.0;
    }
    
    int hammingDistance = 0;
    for (int i = 0; i < hash1.length(); ++i) {
        if (hash1[i] != hash2[i]) {
            hammingDistance++;
        }
    }
    
    // 将汉明距离转换为相似度 (0-1之间)
    double similarity = 1.0 - (static_cast<double>(hammingDistance) / hash1.length());
    return similarity;
}

// 配方识别ROI常量定义
const QRect RecipeRecognizer::RECIPE_ROI(4, 4, 38, 24);

// 执行网格哈希匹配的通用方法
QList<QPair<QPoint, double>> RecipeRecognizer::performGridHashMatching(const QImage& recipeArea, const QString& targetRecipe, 
                                                                       const QVector<int>& xLines, const QVector<int>& yLines)
{
    QList<QPair<QPoint, double>> matches;
    QImage targetTemplate = recipeTemplateImages[targetRecipe];
    
    // 输出当前使用的模板哈希值
    if (recipeTemplateHashes.contains(targetRecipe)) {
        QString currentTemplateHash = recipeTemplateHashes[targetRecipe];
        qDebug() << QString("当前匹配模板 %1 的哈希值: %2").arg(targetRecipe).arg(currentTemplateHash);
    }
    
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
            
            // 使用配方ROI区域进行匹配
            QImage gridROI = gridImage.copy(RECIPE_ROI);
            if (gridROI.isNull()) continue;
            
            QImage templateROI = targetTemplate.copy(RECIPE_ROI);
            if (templateROI.isNull()) continue;
            
            QString gridHash = calculateImageHash(gridROI);
            QString templateHash = calculateImageHash(templateROI);
            double similarity = calculateHashSimilarity(gridHash, templateHash);
            
            // 输出哈希值比较信息
            qDebug() << QString("网格(%1,%2) 哈希比较: 网格哈希=%3, 模板哈希=%4, 相似度=%5")
                       .arg(x0).arg(y0).arg(gridHash).arg(templateHash).arg(QString::number(similarity, 'f', 4));
            
            matches.append(qMakePair(QPoint(x0, y0), similarity));
        }
    }
    
    std::sort(matches.begin(), matches.end(), [](const QPair<QPoint, double>& a, const QPair<QPoint, double>& b) { 
        return a.second > b.second; 
    });
    
    return matches;
}

// 保存匹配调试图像的通用方法
void RecipeRecognizer::saveMatchDebugImages(const QList<QPair<QPoint, double>>& matches, const QImage& recipeArea,
                                           const QVector<int>& xLines, const QVector<int>& yLines, 
                                           const QString& debugDir, const QString& timestamp, int duration)
{
    if (matches.isEmpty()) return;
    
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
    
    // 输出识别结果汇总
    QPoint bestPos = matches[0].first; 
    double bestSim = matches[0].second;
    qDebug() << QString("=== 页面识别结果 ===");
    qDebug() << QString("最佳匹配位置: (%1,%2), 相似度: %3").arg(bestPos.x()).arg(bestPos.y()).arg(QString::number(bestSim, 'f', 4));
    if (matches.size() >= 2) {
        QPoint secondPos = matches[1].first; 
        double secondSim = matches[1].second;
        qDebug() << QString("次佳匹配位置: (%1,%2), 相似度: %3").arg(secondPos.x()).arg(secondPos.y()).arg(QString::number(secondSim, 'f', 4));
    }
    qDebug() << QString("===================");
    
    // 保存最佳匹配图像
    QRect bestGridRect = findGridRect(bestPos);
    if (bestGridRect.isValid()) {
        QImage bestGridImage = recipeArea.copy(bestGridRect);
        QString bestGridPath = debugDir + QString("/best_match_grid_%1_%2.png").arg(QString::number(bestSim, 'f', 4)).arg(timestamp);
        if (bestGridImage.save(bestGridPath)) {
            qDebug() << QString("最佳匹配网格图像已保存: %1").arg(bestGridPath);
            qDebug() << "[DEBUG] 最佳匹配图片路径:" << bestGridPath << " 匹配度:" << bestSim << " 耗时:" << duration << "ms";
        }
    }
    
    // 保存次佳匹配图像
    if (matches.size() >= 2) {
        QPoint secondPos = matches[1].first; 
        double secondSim = matches[1].second;
        QRect secondGridRect = findGridRect(secondPos);
        if (secondGridRect.isValid()) {
            QImage secondGridImage = recipeArea.copy(secondGridRect);
            QString secondGridPath = debugDir + QString("/second_match_grid_%1_%2.png").arg(QString::number(secondSim, 'f', 4)).arg(timestamp);
            if (secondGridImage.save(secondGridPath)) {
                qDebug() << QString("次佳匹配网格图像已保存: %1").arg(secondGridPath);
                qDebug() << "[DEBUG] 次佳匹配图片路径:" << secondGridPath << " 匹配度:" << secondSim << " 耗时:" << duration << "ms";
            }
        }
    }
}

// 加载配方模板
bool RecipeRecognizer::loadRecipeTemplates()
{
    recipeTemplateHashes.clear();
    recipeTemplateImages.clear();
    
    // 从资源文件中动态读取配方模板
    QDir resourceDir(":/images/recipe");
    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    QStringList recipeFiles = resourceDir.entryList(nameFilters, QDir::Files);
    
    if (recipeFiles.isEmpty()) {
        qDebug() << "未找到任何配方模板文件";
        return false;
    }
    
    qDebug() << "找到配方模板文件:" << recipeFiles;
    
    for (const QString& recipeFile : recipeFiles) {
        // 提取配方类型（文件名，不包含扩展名）
        QString recipeType = QFileInfo(recipeFile).baseName();
        QString filePath = QString(":/images/recipe/%1").arg(recipeFile);
        
        QImage template_image(filePath);
        
        if (template_image.isNull()) {
            qDebug() << "无法加载配方模板:" << recipeType << "路径:" << filePath;
            continue;
        }
        
        // 保存模板图像和计算哈希值
        recipeTemplateImages[recipeType] = template_image;
        QString hash = calculateImageHash(template_image);
        recipeTemplateHashes[recipeType] = hash;
        
        // qDebug() << "成功加载配方模板:" << recipeType << "哈希值:" << hash;
    }
    
    recipeTemplatesLoaded = !recipeTemplateHashes.isEmpty();
    if (recipeTemplatesLoaded) {
        qDebug() << "配方模板加载完成，总数:" << recipeTemplateHashes.size();
        
        // 输出所有已加载的模板哈希值
        qDebug() << "=== 所有配方模板哈希值 ===";
        for (auto it = recipeTemplateHashes.begin(); it != recipeTemplateHashes.end(); ++it) {
            qDebug() << QString("模板 %1: %2").arg(it.key()).arg(it.value());
        }
        qDebug() << "============================";
        
        // QStringList loadedTypes = recipeTemplateHashes.keys();
    } else {
        qDebug() << "配方模板加载失败，没有成功加载任何模板";
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
    QStringList types = recipeTemplateHashes.keys();
    std::sort(types.begin(), types.end());
    return types;
}

// 识别配方
QPair<QString, double> RecipeRecognizer::recognizeRecipe(const QImage& recipeArea)
{
    if (!recipeTemplatesLoaded) {
        qDebug() << "配方模板未加载，无法进行识别";
        return qMakePair("", 0.0);
    }
    
    if (recipeArea.isNull()) {
        qDebug() << "配方区域图像无效";
        return qMakePair("", 0.0);
    }
    
    // 计算当前配方区域的哈希值
    QString currentHash = calculateImageHash(recipeArea);
    
    QString bestMatch = "";
    double bestSimilarity = 0.0;
    QString secondBestMatch = "";
    double secondBestSimilarity = 0.0;
    QString bestMatchHash = "";
    QString secondBestMatchHash = "";
    
    // 与所有模板进行比较
    for (auto it = recipeTemplateHashes.begin(); it != recipeTemplateHashes.end(); ++it) {
        const QString& recipeType = it.key();
        const QString& templateHash = it.value();
        
        // 计算相似度
        double similarity = calculateHashSimilarity(currentHash, templateHash);
        
        qDebug() << "配方与" << recipeType << "的哈希相似度:" << QString::number(similarity, 'f', 4) 
                 << "模板哈希:" << templateHash << "当前哈希:" << currentHash;
        
        // 更新最佳匹配和次佳匹配
        if (similarity > bestSimilarity) {
            secondBestSimilarity = bestSimilarity;
            secondBestMatch = bestMatch;
            secondBestMatchHash = bestMatchHash;
            bestSimilarity = similarity;
            bestMatch = recipeType;
            bestMatchHash = templateHash;
        } else if (similarity > secondBestSimilarity) {
            secondBestSimilarity = similarity;
            secondBestMatch = recipeType;
            secondBestMatchHash = templateHash;
        }
    }
    
    // 输出详细的匹配调试信息
    qDebug() << "=== 配方识别结果详情 ===";
    qDebug() << "当前配方哈希值:" << currentHash;
    qDebug() << "最佳匹配配方:" << bestMatch << "相似度:" << QString::number(bestSimilarity, 'f', 4) << "哈希值:" << bestMatchHash;
    qDebug() << "次佳匹配配方:" << secondBestMatch << "相似度:" << QString::number(secondBestSimilarity, 'f', 4) << "哈希值:" << secondBestMatchHash;
    qDebug() << "=========================";
    
    return qMakePair(bestMatch, bestSimilarity);
}

// 在网格中找最佳匹配
QList<QPair<QPoint, double>> RecipeRecognizer::findBestMatchesInGrid(const QImage& recipeArea, const QString& targetRecipe)
{
    QList<QPair<QPoint, double>> matches;
    
    if (!recipeTemplatesLoaded || !recipeTemplateImages.contains(targetRecipe)) {
        qDebug() << QString("配方模板 %1 未加载").arg(targetRecipe);
        return matches;
    }
    
    if (recipeArea.isNull()) {
        qDebug() << "配方区域图像无效";
        return matches;
    }
    
    QImage targetTemplate = recipeTemplateImages[targetRecipe];
    
    // 根据49x49的网格分割配方区域
    const int gridSize = 49;
    const int cols = recipeArea.width() / gridSize;  // 365 / 49 ≈ 7列
    const int rows = recipeArea.height() / gridSize; // 200 / 49 ≈ 4行
    
    qDebug() << QString("网格分割: %1行 x %2列").arg(rows).arg(cols);
    
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
            QString gridHash = calculateImageHash(gridImage);
            QString targetHash = recipeTemplateHashes[targetRecipe];
            double similarity = calculateHashSimilarity(gridHash, targetHash);
            
            // 记录网格位置和相似度
            QPoint gridPos(col, row);
            matches.append(qMakePair(gridPos, similarity));
            
            qDebug() << QString("网格(%1,%2) 与模板%3的哈希相似度: %4")
                        .arg(col).arg(row).arg(targetRecipe).arg(QString::number(similarity, 'f', 4));
        }
    }
    
    // 按相似度排序，找出最佳匹配
    std::sort(matches.begin(), matches.end(), 
              [](const QPair<QPoint, double>& a, const QPair<QPoint, double>& b) {
                  return a.second > b.second;
              });
    
    // 匹配结果将通过saveMatchDebugImages输出
    
    return matches;
}

// 在网格中识别配方
void RecipeRecognizer::recognizeRecipeInGrid(const QImage& screenshot, const QString& targetRecipe, HWND hwndGame)
{
    qDebug() << "开始配方识别...";
    
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
            qDebug() << QString("配方区域图像已保存: %1").arg(recipeImagePath);
        }
        
        addLog(QString("选择匹配模板: %1").arg(targetRecipe));
        
        // 获取分割线坐标
        QVector<int> xLines, yLines;
        getRecipeGridLines(recipeArea, xLines, yLines);
        addLog(QString("检测到横线y: %1, 纵线x: %2").arg(yLines.size()).arg(xLines.size()));
        
        // 执行配方识别
        auto startTime = std::chrono::high_resolution_clock::now();
        QList<QPair<QPoint, double>> matches = performGridHashMatching(recipeArea, targetRecipe, xLines, yLines);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        qDebug() << QString("配方识别耗时: %1 毫秒").arg(duration.count());
        
        // 保存最佳匹配的网格图像
        saveMatchDebugImages(matches, recipeArea, xLines, yLines, debugDir, timestamp, duration.count());
        
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
            qDebug() << QString("点击配方中心位置: (%1, %2), 相似度: %3").arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4));
        }
    }
}

// 带翻页功能的配方识别
void RecipeRecognizer::recognizeRecipeWithPaging(const QImage& screenshot, const QString& targetRecipe, HWND hwndGame)
{
    qDebug() << "开始带翻页功能的配方识别...";
    
    // 确保输出目标配方模板的哈希值信息
    if (recipeTemplateHashes.contains(targetRecipe)) {
        qDebug() << QString("目标配方 %1 模板哈希: %2").arg(targetRecipe).arg(recipeTemplateHashes[targetRecipe]);
    } else {
        qDebug() << QString("警告: 目标配方 %1 模板未找到!").arg(targetRecipe);
    }
    
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
        
        QList<QPair<QPoint, double>> matches = performGridHashMatching(recognitionArea, targetRecipe, xLines, yLines);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // 保存调试图像
        saveMatchDebugImages(matches, recipeArea, xLines, yLines, debugDir, timestamp, duration.count());
        
        return matches;
    };

    // 步骤1: 先识别当前页面的配方
            qDebug() << "先识别当前页面的配方...";
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
            qDebug() << QString("在当前页面找到配方并点击: (%1, %2), 相似度: %3").arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4));
            return;
        }
    }
    
    // 步骤2: 当前页面没找到，翻到顶部重新识别
            qDebug() << "当前页面未找到配方，翻到顶部重新识别...";
    // 翻到顶
            qDebug() << "翻到顶...";
    // 修正：点击配方区域内的(355, 20)（相对于全屏为(555+355, 88+20)）
    int clickTopX = recipeX + 355;
    int clickTopY = recipeY + 20;
    leftClickDPI(hwndGame, clickTopX, clickTopY);
            qDebug() << QString("[翻页] 点击配方区域顶部: 全屏坐标(%1, %2)").arg(clickTopX).arg(clickTopY);
    qDebug() << "[翻页] 点击配方区域顶部: 全屏坐标(" << clickTopX << "," << clickTopY << ")";
    sleepByQElapsedTimer(300);

    // 翻页到顶部后，重新截取游戏窗口
    QImage topScreenshot = captureWindowByHandle(hwndGame,"主页面");
    if (topScreenshot.isNull()) {
        qDebug() << "翻页到顶部后截图失败";
        return;
    }
            qDebug() << "翻页到顶部后重新截取游戏窗口";

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
            qDebug() << QString("找到配方并点击: (%1, %2), 相似度: %3").arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4));
            return;
        }
    }
    
    // 翻页查找
    if (!foundRecipe) {
        qDebug() << "开始翻页查找配方...";
        int pageCount = 0, maxPages = 21;
        while (pageCount < maxPages) {
            // 修正：点击配方区域内的(355, 190)（相对于全屏为(555+355, 88+190)）
            int clickPageX = recipeX + 355;
            int clickPageY = recipeY + 190;
            leftClickDPI(hwndGame, clickPageX, clickPageY);
            qDebug() << QString("[翻页] 点击配方区域底部: 全屏坐标(%1, %2)").arg(clickPageX).arg(clickPageY);
            qDebug() << "[翻页] 点击配方区域底部: 全屏坐标(" << clickPageX << "," << clickPageY << ")";
            sleepByQElapsedTimer(300);
            pageCount++;
            qDebug() << QString("翻到第 %1 页").arg(pageCount);
            QImage newScreenshot = captureWindowByHandle(hwndGame,"主页面");
            if (newScreenshot.isNull()) { qDebug() << "截图失败"; break; }
            QString timestamp3 = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
            QList<QPair<QPoint, double>> newMatches = recognizeOnePage(newScreenshot, timestamp3);
            if (!newMatches.isEmpty() && newMatches[0].second >= 1.0) {
                QPoint bestPos = newMatches[0].first;
                if (bestPos.y() < recogH) {
                    double bestSim = newMatches[0].second;
                    int centerX = bestPos.x() + 24, centerY = bestPos.y() + 24;
                    int screenX = recipeX + centerX, screenY = recipeY + centerY;
                    leftClickDPI(hwndGame, screenX, screenY);
                    qDebug() << QString("在第 %1 页找到配方并点击: (%2, %3), 相似度: %4").arg(pageCount).arg(screenX).arg(screenY).arg(QString::number(bestSim, 'f', 4));
                    return;
                }
            }
        }
        qDebug() << "翻页完成，未找到目标配方";
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
