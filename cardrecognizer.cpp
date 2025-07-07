#include "cardrecognizer.h"
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <chrono>
#include <QDateTime>
#include <QPainter>
#include <QPen>

// 获取应用程序数据目录
QString getAppDataPath() {
    QDir appDir(QCoreApplication::applicationDirPath());
    
    // 创建数据目录
    if (!appDir.exists("data")) {
        appDir.mkdir("data");
    }
    
    // 创建截图目录
    QDir dataDir(appDir.absoluteFilePath("data"));
    if (!dataDir.exists("screenshots")) {
        dataDir.mkdir("screenshots");
    }
    
    return dataDir.absolutePath();
}

// 用于调试输出哈希值
QString hashToString(const cv::Mat& hash) {
    QString result;
    for (int i = 0; i < hash.cols; i++) {
        result += QString::number(hash.at<uchar>(0, i)) + " ";
    }
    return result.trimmed();
}

CardRecognizer::CardRecognizer(QObject *parent)
    : QObject(parent)
{
    qDebug() << "初始化 CardRecognizer...";
    qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    qDebug() << "Data directory:" << getAppDataPath();
    qDebug() << "Card Type ROI dimensions:" << CARD_TYPE_ROI_WIDTH << "x" << CARD_TYPE_ROI_HEIGHT;
    qDebug() << "Card Level ROI dimensions:" << CARD_LEVEL_ROI_WIDTH << "x" << CARD_LEVEL_ROI_HEIGHT;
    qDebug() << "Card Bind ROI dimensions:" << CARD_BIND_ROI_WIDTH << "x" << CARD_BIND_ROI_HEIGHT;
    
    if (!loadTemplates()) {
        qDebug() << "模板加载失败，卡片识别功能将无法工作！";
        qDebug() << "Please ensure all required files exist in the following structure:";
        qDebug() << "- items/position/line.png (separator line template)";
        qDebug() << "- items/card/*.png (card templates)";
    }
    
    // 加载星级模板
    loadLevelTemplates();
    
    // 加载绑定模板
    loadBindTemplate();
}

bool CardRecognizer::loadTemplates()
{
    // 加载分隔线模板
    QString linePath = ":/items/position/line.png";
    
    // 先将资源文件加载为QImage
    QImage lineImage(linePath);
    if (lineImage.isNull()) {
        qDebug() << "分隔线模板加载失败:" << linePath;
        return false;
    }
    
    // 转换为Mat格式
    separatorLine = qImageToCvMat(lineImage);
    if (separatorLine.empty()) {
        qDebug() << "分隔线模板转换失败";
        return false;
    }

    // 加载卡片模板
    QDir resourceDir(":/items/card");
    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    QStringList cardFiles = resourceDir.entryList(nameFilters, QDir::Files);
    qDebug() << "发现" << cardFiles.size() << "个模板文件:" << cardFiles;

    for (const QString& cardFile : cardFiles) {
        QString fullPath = QString(":/items/card/%1").arg(cardFile);
        
        try {
            // 先加载为QImage
            QImage cardImage(fullPath);
            if (cardImage.isNull()) {
                qDebug() << "模板加载失败:" << fullPath;
                continue;
            }
            
            // 转换为Mat格式
            cv::Mat cardTemplate = qImageToCvMat(cardImage);
            if (cardTemplate.empty()) {
                qDebug() << "模板转换失败:" << fullPath;
                continue;
            }

            // 提取模板的ROI区域
            if (cardTemplate.cols < CARD_TYPE_ROI_X + CARD_TYPE_ROI_WIDTH || cardTemplate.rows < CARD_TYPE_ROI_Y + CARD_TYPE_ROI_HEIGHT) {
                qDebug() << "模板尺寸过小:" << cardTemplate.cols << "x" << cardTemplate.rows;
                continue;
            }

            cv::Mat roiTemplate = cardTemplate(cv::Rect(CARD_TYPE_ROI_X, CARD_TYPE_ROI_Y, CARD_TYPE_ROI_WIDTH, CARD_TYPE_ROI_HEIGHT)).clone();
            if (roiTemplate.empty()) {
                qDebug() << "ROI提取失败:" << cardFile;
                continue;
            }
            
            // 计算ROI的哈希值
            cv::Mat hash = CardTemplateManager::computeHash(roiTemplate);
            if (hash.empty()) {
                qDebug() << "哈希计算失败:" << cardFile;
                continue;
            }
            
            // 去掉文件扩展名作为卡片名称
            QString cardName = QFileInfo(cardFile).baseName();
            templateManager.addTemplate(cardName.toStdString(), hash);
            
            qDebug() << "Successfully loaded and hashed template:" << cardName
                     << "\nTemplate size:" << cardTemplate.cols << "x" << cardTemplate.rows
                     << "\nROI size:" << roiTemplate.cols << "x" << roiTemplate.rows
                     << "\nHash size:" << hash.cols << "x" << hash.rows
                     << "\nHash value:" << hashToString(hash);

            // 保存调试图像
            QString debugDir = getAppDataPath() + "/template_debug";
            QDir().mkpath(debugDir);
            
            // 保存ROI
            cv::imwrite(QString("%1/%2_roi.png").arg(debugDir).arg(cardName).toStdString(), roiTemplate);
            
            // 保存哈希值可视化
            cv::Mat hashVis = cv::Mat::zeros(8, 8, CV_8UC1);
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    hashVis.at<uchar>(i, j) = hash.at<uchar>(0, i * 8 + j) * 255;
                }
            }
            cv::imwrite(QString("%1/%2_hash.png").arg(debugDir).arg(cardName).toStdString(), hashVis);
        }
        catch (const cv::Exception& e) {
            qDebug() << "模板处理失败" << cardFile << ":" << e.what();
            continue;
        }
    }

    auto cards = templateManager.getRegisteredCards();
    if (cards.empty()) {
        qDebug() << "没有加载任何卡片模板";
        return false;
    }

    qDebug() << "成功加载" << cards.size() << "个模板";
    return true;
}

cv::Mat CardRecognizer::qImageToCvMat(const QImage& image)
{
    switch (image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied: {
        cv::Mat mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        cv::Mat mat2;
        cv::cvtColor(mat, mat2, cv::COLOR_BGRA2BGR);
        return mat2;
    }
    case QImage::Format_RGB888: {
        cv::Mat mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::Mat mat2;
        cv::cvtColor(mat, mat2, cv::COLOR_RGB2BGR);
        return mat2;
    }
    default:
        QImage converted = image.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat(converted.height(), converted.width(), CV_8UC3, (void*)converted.constBits(), converted.bytesPerLine());
        cv::Mat mat2;
        cv::cvtColor(mat, mat2, cv::COLOR_RGB2BGR);
        return mat2;
    }
}

QImage CardRecognizer::cvMatToQImage(const cv::Mat& mat)
{
    // 确保图像不为空
    if (mat.empty()) {
        return QImage();
    }
    
    // 根据通道数处理
    if (mat.channels() == 3) {
        // BGR格式转为RGB
        cv::Mat rgbMat;
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        return QImage(rgbMat.data, rgbMat.cols, rgbMat.rows, rgbMat.step, QImage::Format_RGB888).copy();
    } else if (mat.channels() == 1) {
        // 灰度图像
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    } else if (mat.channels() == 4) {
        // BGRA格式
        cv::Mat rgbaMat;
        cv::cvtColor(mat, rgbaMat, cv::COLOR_BGRA2RGBA);
        return QImage(rgbaMat.data, rgbaMat.cols, rgbaMat.rows, rgbaMat.step, QImage::Format_RGBA8888).copy();
    }
    
    return QImage();
}

void CardRecognizer::loadLevelTemplates()
{
    m_levelTemplates.clear();
    
    // 按1-16的顺序加载星级模板
    for (int level = 1; level <= 16; ++level) {
        QString levelStr = QString::number(level);
        QString filePath = QString(":/items/level/%1.png").arg(level);
        
        QImage levelImage(filePath);
        if (!levelImage.isNull()) {
            m_levelTemplates[levelStr] = levelImage;
            qDebug() << "Loaded level template:" << levelStr;
        } else {
            qWarning() << "Failed to load level template:" << filePath;
        }
    }
}

void CardRecognizer::loadBindTemplate()
{
    QString filePath = ":/items/bind_state/card_bind.png";
    m_bindTemplate = QImage(filePath);
    
    if (!m_bindTemplate.isNull()) {
        qDebug() << "Loaded bind template successfully";
        qDebug() << "Bind template size:" << m_bindTemplate.width() << "x" << m_bindTemplate.height();
    } else {
        qWarning() << "Failed to load bind template:" << filePath;
    }
}

int CardRecognizer::recognizeCardLevel(const cv::Mat& cardMat)
{
    // 检查卡片尺寸是否足够
    if (cardMat.cols < CARD_LEVEL_ROI_X + CARD_LEVEL_ROI_WIDTH || 
        cardMat.rows < CARD_LEVEL_ROI_Y + CARD_LEVEL_ROI_HEIGHT) {
        qWarning() << "Card size too small for level ROI:" << cardMat.cols << "x" << cardMat.rows;
        return 0;
    }
    
    // 提取星级ROI区域
    cv::Mat levelRoi = cardMat(cv::Rect(CARD_LEVEL_ROI_X, CARD_LEVEL_ROI_Y, 
                                       CARD_LEVEL_ROI_WIDTH, CARD_LEVEL_ROI_HEIGHT));
    
    // 保存ROI图片用于调试
    // QString debugPath = "error/level_roi_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";
    // QImage debugImage = cvMatToQImage(levelRoi);
    // debugImage.save(debugPath);
    // qDebug() << "Saved level ROI for debugging:" << debugPath;

    // 按1-16顺序检查每个星级模板，找到完全匹配时立即退出
    int recognizedLevel = 0;

    for (int level = 1; level <= 16; ++level) {
        QString levelStr = QString::number(level);
        
        // if (!m_levelTemplates.contains(levelStr)) {
        //     qWarning() << "Level template not found:" << levelStr;
        //     continue;
        // }
        
        const QImage& templateImg = m_levelTemplates[levelStr];
        
        // 转换模板为cv::Mat
        cv::Mat templateMat = qImageToCvMat(templateImg);
        
        // 确保尺寸匹配
        // if (templateMat.rows != levelRoi.rows || templateMat.cols != levelRoi.cols) {
        //     qWarning() << "Template size mismatch for level" << levelStr 
        //               << "Template:" << templateMat.rows << "x" << templateMat.cols
        //               << "ROI:" << levelRoi.rows << "x" << levelRoi.cols;
        //     continue;
        // }

        // 计算图像哈希匹配度
        cv::Mat levelHash = CardTemplateManager::computeHash(levelRoi);
        cv::Mat templateHash = CardTemplateManager::computeHash(templateMat);
        double matchScore = CardTemplateManager::compareHash(levelHash, templateHash);
        // qDebug() << "Level" << levelStr << "match score:" << matchScore;

        // 如果找到完全匹配（匹配度为1），立即返回结果
        if (matchScore == 1.0) {
            recognizedLevel = level;
            // qDebug() << "Found perfect match for level" << level << ", stopping search";
            break;
        }
    }

    qDebug() << "Recognized card level:" << recognizedLevel;
    return recognizedLevel;
}

bool CardRecognizer::recognizeCardBind(const cv::Mat& cardMat)
{
    // 检查卡片尺寸是否足够
    if (cardMat.cols < CARD_BIND_ROI_X + CARD_BIND_ROI_WIDTH || 
        cardMat.rows < CARD_BIND_ROI_Y + CARD_BIND_ROI_HEIGHT) {
        qWarning() << "Card size too small for bind ROI:" << cardMat.cols << "x" << cardMat.rows;
        return false;
    }
    
    // 检查绑定模板是否加载
    if (m_bindTemplate.isNull()) {
        qWarning() << "Bind template not loaded";
        return false;
    }
    
    // 提取绑定ROI区域
    cv::Mat bindRoi = cardMat(cv::Rect(CARD_BIND_ROI_X, CARD_BIND_ROI_Y, 
                                      CARD_BIND_ROI_WIDTH, CARD_BIND_ROI_HEIGHT));
    
    // 保存ROI图片用于调试
    // QString debugPath = "error/bind_roi_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".png";
    // QImage debugImage = cvMatToQImage(bindRoi);
    // debugImage.save(debugPath);
    // qDebug() << "Saved bind ROI for debugging:" << debugPath;
    
    // 转换模板为cv::Mat
    cv::Mat templateMat = qImageToCvMat(m_bindTemplate);
    
    // 确保尺寸匹配
    if (templateMat.rows != bindRoi.rows || templateMat.cols != bindRoi.cols) {
        qWarning() << "Template size mismatch for bind state"
                  << "Template:" << templateMat.rows << "x" << templateMat.cols
                  << "ROI:" << bindRoi.rows << "x" << bindRoi.cols;
        return false;
    }
    
    // 计算图像哈希匹配度
    cv::Mat bindHash = CardTemplateManager::computeHash(bindRoi);
    cv::Mat templateHash = CardTemplateManager::computeHash(templateMat);
    double matchScore = CardTemplateManager::compareHash(bindHash, templateHash);
    qDebug() << "Bind match score:" << matchScore;
    
    // 只有完全匹配（匹配度为1）才认为是绑定状态
    bool isBound = (matchScore == 1.0);
    qDebug() << "Recognized bind state:" << (isBound ? "Bound" : "Unbound");
    
    return isBound;
}

// CardTemplateManager implementation
void CardTemplateManager::addTemplate(const std::string& cardName, const cv::Mat& templateImage) {
    templates[cardName] = templateImage.clone();
    templateHashes[cardName] = computeHash(templateImage);
}

cv::Mat CardTemplateManager::getTemplate(const std::string& cardName) const {
    auto it = templates.find(cardName);
    return (it != templates.end()) ? it->second : cv::Mat();
}

std::vector<std::string> CardTemplateManager::getRegisteredCards() const {
    std::vector<std::string> cards;
    for (const auto& pair : templates) {
        cards.push_back(pair.first);
    }
    return cards;
}

std::vector<std::string> CardRecognizer::getRegisteredCards() const
{
    return templateManager.getRegisteredCards();
}

cv::Mat CardTemplateManager::computeHash(const cv::Mat& img) {
    try {
        cv::Mat resized, gray;
        
        // 确保图像是灰度的
        if (img.channels() == 3) {
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = img.clone();
        }
        
        // 计算每个8x8区块的平均值
        const int BLOCK_ROWS = 8;
        const int BLOCK_COLS = 8;
        const int blockHeight = std::max(1, gray.rows / BLOCK_ROWS);
        const int blockWidth = std::max(1, gray.cols / BLOCK_COLS);
        
        // 创建64位哈希值
        cv::Mat hash = cv::Mat::zeros(1, 64, CV_8UC1);
        double totalMean = cv::mean(gray)[0];
        
        int idx = 0;
        for (int i = 0; i < BLOCK_ROWS && idx < 64; i++) {
            for (int j = 0; j < BLOCK_COLS && idx < 64; j++) {
                // 计算当前块的范围
                int startY = i * blockHeight;
                int startX = j * blockWidth;
                int endY = std::min(gray.rows, (i + 1) * blockHeight);
                int endX = std::min(gray.cols, (j + 1) * blockWidth);
                
                if (startY >= endY || startX >= endX) {
                    hash.at<uchar>(0, idx++) = 0;
                    continue;
                }
                
                // 提取当前块并计算平均值
                cv::Mat block = gray(cv::Range(startY, endY), cv::Range(startX, endX));
                if (!block.empty()) {
                    double blockMean = cv::mean(block)[0];
                    hash.at<uchar>(0, idx++) = (blockMean > totalMean) ? 1 : 0;
                } else {
                    hash.at<uchar>(0, idx++) = 0;
                }
            }
        }
        
        return hash;
    }
    catch (const cv::Exception& e) {
        qDebug() << "Error in computeHash:" << e.what();
        return cv::Mat();
    }
}

double CardTemplateManager::compareHash(const cv::Mat& hash1, const cv::Mat& hash2) {
    if (hash1.empty() || hash2.empty() || hash1.cols != hash2.cols) {
        return 0.0;
    }
    
    int matches = 0;
    for (int i = 0; i < hash1.cols; i++) {
        if (hash1.at<uchar>(0, i) == hash2.at<uchar>(0, i)) {
            matches++;
        }
    }
    
    return static_cast<double>(matches) / hash1.cols;
}

QPoint CardRecognizer::calculateCardCenterPosition(int row, int col) const
{
    // 计算卡片在背包区域内的中心位置
    int cardCenterX = col * CARD_WIDTH + CARD_WIDTH / 2;
    int cardCenterY = row * CARD_HEIGHT + CARD_HEIGHT / 2;
    
    // 转换为游戏窗口坐标系（需要加上背包区域的偏移）
    int windowX = CARD_AREA.x + cardCenterX;
    int windowY = CARD_AREA.y + cardCenterY;
    
    return QPoint(windowX, windowY);
}

std::vector<CardInfo> CardRecognizer::recognizeCardsDetailed(const QImage& screenshot, const QStringList& targetCardTypes)
{
    std::vector<CardInfo> results;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    cv::Mat img = qImageToCvMat(screenshot);
    
    if (img.empty()) {
        qDebug() << "截图转换失败";
        return results;
    }

    // 将目标卡片类型转换为std::vector<std::string>
    std::vector<std::string> targetCards;
    for (const QString& cardType : targetCardTypes) {
        targetCards.push_back(cardType.toStdString());
    }
    
    qDebug() << "识别目标卡片类型:" << targetCardTypes.size() << "种";

    try {
        // 提取卡片区域
        cv::Mat cardArea = img(CARD_AREA).clone();
        
        // 在限制范围内查找顶部分隔线位置
        cv::Mat searchArea = cardArea(cv::Rect(0, 0, cardArea.cols, std::min(MAX_SEPARATOR_SEARCH_HEIGHT, cardArea.rows)));
        cv::Mat result;
        cv::matchTemplate(searchArea, separatorLine, result, cv::TM_CCOEFF_NORMED);
        
        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
        
        if (maxVal < MATCH_THRESHOLD) {
            qDebug() << "未找到顶部分隔线，匹配度:" << maxVal;
            return results;
        }

        qDebug() << "找到分隔线，位置:" << maxLoc.y << "，匹配度:" << maxVal;

        // 截取分隔线下方的卡片区域
        int startY = maxLoc.y + separatorLine.rows;
        int availableHeight = cardArea.rows - startY;
        int processHeight = std::min(availableHeight, CARD_AREA_HEIGHT);
        
        if (processHeight <= 0) {
            qDebug() << "分隔线下方无可用空间";
            return results;
        }
        
        cv::Mat cardsArea = cardArea(cv::Rect(0, startY, cardArea.cols, processHeight)).clone();

        // 保存卡片区域图像
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QString cardsAreaPath = QString("screenshots/cardsarea_%1.png").arg(timestamp);
        cv::imwrite(cardsAreaPath.toStdString(), cardsArea);
        qDebug() << "Card area saved to:" << cardsAreaPath;
        
        // 计算实际可处理的行数
        int maxRows = processHeight / CARD_HEIGHT;

        // 遍历每个卡片位置
        for (int row = 0; row < std::min(TOTAL_ROWS, maxRows); ++row) {
            for (int col = 0; col < CARDS_PER_ROW; ++col) {
                // 计算当前卡片的位置
                int cardX = col * CARD_WIDTH;
                int cardY = row * CARD_HEIGHT;
                
                // 确保不会越界
                if (cardX + CARD_WIDTH > cardsArea.cols || cardY + CARD_HEIGHT > cardsArea.rows) {
                    continue;
                }

                // 计算ROI区域
                int roiX = cardX + CARD_TYPE_ROI_X;
                int roiY = cardY + CARD_TYPE_ROI_Y;
                
                if (roiX + CARD_TYPE_ROI_WIDTH > cardsArea.cols || roiY + CARD_TYPE_ROI_HEIGHT > cardsArea.rows) {
                    continue;
                }

                try {
                    // 提取当前卡片的ROI区域
                    cv::Mat cardRoi = cardsArea(cv::Rect(roiX, roiY, CARD_TYPE_ROI_WIDTH, CARD_TYPE_ROI_HEIGHT)).clone();
                    
                    if (cardRoi.empty()) {
                        continue;
                    }

                    // 计算当前ROI的哈希值
                    cv::Mat currentHash = CardTemplateManager::computeHash(cardRoi);
                    if (currentHash.empty()) {
                        continue;
                    }

                    // 只与目标卡片类型进行匹配
                    std::string bestMatch = "";
                    double bestSimilarity = 0.0;
                    
                    for (const std::string& targetCard : targetCards) {
                        cv::Mat templateHash = templateManager.getTemplate(targetCard);
                        if (templateHash.empty()) {
                            continue; // 如果模板不存在，跳过
                        }
                        
                        double similarity = CardTemplateManager::compareHash(currentHash, templateHash);
                        
                        if (similarity > bestSimilarity) {
                            bestSimilarity = similarity;
                            bestMatch = targetCard;
                        }
                    }
                    
                    // 只有相似度达到阈值才认为是有效匹配
                    if (bestSimilarity >= 1.0 && !bestMatch.empty()) {
                        // 提取整个卡片用于星级和绑定状态识别
                        cv::Mat fullCard = cardsArea(cv::Rect(cardX, cardY, CARD_WIDTH, CARD_HEIGHT));
                        
                        // 识别卡片星级
                        int cardLevel = recognizeCardLevel(fullCard);
                        
                        // 识别卡片绑定状态
                        bool isBound = recognizeCardBind(fullCard);
                        
                        // 计算卡片中心位置（需要考虑分隔线偏移）
                        QPoint centerPos = calculateCardCenterPosition(row, col);
                        // 调整Y坐标，加上分隔线的偏移
                        centerPos.setY(centerPos.y() + startY);
                        
                        // 创建CardInfo对象
                        CardInfo cardInfo(bestMatch, cardLevel, isBound, centerPos, row, col);
                        results.push_back(cardInfo);
                        
                        qDebug() << "找到目标卡片:" << QString::fromStdString(bestMatch) 
                                << "星级:" << cardLevel
                                << "绑定状态:" << (isBound ? "已绑定" : "未绑定")
                                << "位置:" << row << "," << col
                                << "中心坐标:" << centerPos.x() << "," << centerPos.y();
                    }

                } catch (const cv::Exception& e) {
                    qDebug() << "处理卡片ROI时发生错误:" << e.what();
                }
            }
        }

    } catch (const cv::Exception& e) {
        qDebug() << "识别过程中发生错误:" << e.what();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    qDebug() << "针对性详细卡片识别完成，用时:" << duration.count() << "ms，识别到" << results.size() << "张卡片";

    return results;
} 