#include "cardrecognizer.h"
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <chrono>
#include <QDateTime>
#include <QPainter>
#include <QPen>
#include <QColor>

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

CardRecognizer::CardRecognizer(QObject *parent)
    : QObject(parent)
{
    qDebug() << "初始化 CardRecognizer...";
    // qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    // qDebug() << "Data directory:" << getAppDataPath();
    // qDebug() << "Card Type ROI dimensions:" << CARD_TYPE_ROI_WIDTH << "x" << CARD_TYPE_ROI_HEIGHT;
    // qDebug() << "Card Level ROI dimensions:" << CARD_LEVEL_ROI_WIDTH << "x" << CARD_LEVEL_ROI_HEIGHT;
    // qDebug() << "Card Bind ROI dimensions:" << CARD_BIND_ROI_WIDTH << "x" << CARD_BIND_ROI_HEIGHT;
    
    if (!loadTemplates()) {
        qDebug() << "模板加载失败，卡片识别功能将无法工作！";
        qDebug() << "Please ensure all required files exist in the following structure:";
        qDebug() << "- items/position/line.png (separator line template)";
    }
    
    // 加载星级模板
    loadLevelTemplates();
    loadCardLevelHashes();
    
    // 加载绑定模板
    loadBindTemplate();
    loadCardBindHashes();
}

bool CardRecognizer::loadTemplates()
{
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
            // 去掉文件扩展名作为卡片名称
            QString cardName = QFileInfo(cardFile).baseName();

            cardTypeHashes.insert(cardName, calculateImageHash(cardImage, CARD_TYPE_ROI));

            // 保存调试图像
            // QString debugDir = getAppDataPath() + "/template_debug";
            // QDir().mkpath(debugDir);            
        }
        catch (const std::exception& e) {
            qDebug() << "模板处理失败" << cardFile << ":" << e.what();
            continue;
        }
    }

    auto cards = cardTypeHashes.keys();
    if (cards.empty()) {
        qDebug() << "没有加载任何卡片模板";
        return false;
    }

    qDebug() << "成功加载" << cardTypeHashes.size() << "个模板";
    return true;
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
            // qDebug() << "Loaded level template:" << levelStr;
        } else {
            qWarning() << "Failed to load level template:" << filePath;
        }
    }
}

void CardRecognizer::loadCardLevelHashes()
{
    cardLevelHashes.clear();
    for (int level = 1; level <= 16; ++level) {
        QString levelStr = QString::number(level);
        QString filePath = QString(":/items/level/%1.png").arg(levelStr);
        QImage levelImage(filePath);
        if (!levelImage.isNull()) {
            cardLevelHashes.append(calculateImageHash(levelImage));
        } else {
            qWarning() << "Failed to load level template:" << filePath;
        }
    }
    qDebug() << "level 0 hashes:" << cardLevelHashes[0];
}

QString CardRecognizer::calculateImageHash(const QImage& image, const QRect& roi)
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

void CardRecognizer::loadBindTemplate()
{
    QString filePath = ":/items/bind_state/card_bind.png";
    m_bindTemplate = QImage(filePath);
    
    if (!m_bindTemplate.isNull()) {
        // qDebug() << "Loaded bind template successfully";
        // qDebug() << "Bind template size:" << m_bindTemplate.width() << "x" << m_bindTemplate.height();
    } else {
        qWarning() << "Failed to load bind template:" << filePath;
    }
}

void CardRecognizer::loadCardBindHashes()
{
    QString filePath = ":/items/bind_state/card_bind.png";
    QImage bindImage(filePath);
    cardBindHash = calculateImageHash(bindImage);
    qDebug() << "card bind hashes:" << cardBindHash;
}

int CardRecognizer::findStartYUsingColorDetection(const QImage& cardAreaImage)
{
    // 目标颜色 #002D51 (RGB: 0, 45, 81)
    QColor targetColor(0, 45, 81);
    int targetRgb = targetColor.rgb();
    
    // 从第一行开始检查，但限制最大搜索范围为卡片的高度+3
    for (int y = 0; y < 60; ++y) {
        // 检查第47个像素（x=46，因为索引从0开始）
        if (46 < cardAreaImage.width()) {
            QColor pixelColor = cardAreaImage.pixelColor(46, y);
            
            if (pixelColor.rgb() == targetRgb) {
                // 向右检查3个像素
                bool allMatch = true;
                for (int x = 47; x < 50 && x < cardAreaImage.width(); ++x) {
                    QColor rightPixelColor = cardAreaImage.pixelColor(x, y);
                    if (rightPixelColor.rgb() != targetRgb) {
                        allMatch = false;
                        break;
                    }
                }
                
                if (allMatch) {
                    // 找到分隔线，返回该位置+1
                    qDebug() << "通过颜色检测找到分隔线，y坐标:" << y;
                    return y + 1;
                }
            }
        }
    }
    
    qDebug() << "未通过颜色检测找到分隔线";
    return -1;
}

int CardRecognizer::recognizeCardLevel(QImage cardArea)
{
    // 按1-16顺序检查每个星级模板，找到完全匹配时立即退出
    int recognizedLevel = 0;

    for (int level = 0; level < cardLevelHashes.size(); ++level) {
        QString levelHash = calculateImageHash(cardArea, CARD_LEVEL_ROI);
        if (levelHash == cardLevelHashes[level]) {
            recognizedLevel = level + 1;
            break;
        }
    }

    qDebug() << "Recognized card level:" << recognizedLevel;
    return recognizedLevel;
}

bool CardRecognizer::recognizeCardBind(QImage cardArea)
{
    // 计算图像哈希匹配度
    QString cardBindAreaHash = calculateImageHash(cardArea, CARD_BOUND_ROI);
    
    // 只有完全匹配才认为是绑定状态
    bool isBound = (cardBindAreaHash == cardBindHash);
    qDebug() << "Recognized bind state:" << (isBound ? "Bound" : "Unbound");
    
    return isBound;
}

QStringList CardRecognizer::getRegisteredCards() const
{
    return cardTypeHashes.keys();
}

QPoint CardRecognizer::calculateCardCenterPosition(int row, int col) const
{
    // 计算卡片在背包区域内的中心位置
    int cardCenterX = col * CARD_WIDTH + CARD_WIDTH / 2;
    int cardCenterY = row * CARD_HEIGHT + CARD_HEIGHT / 2;
    
    // 转换为游戏窗口坐标系（需要加上背包区域的偏移）
    int windowX = CARD_AREA.x() + cardCenterX;
    int windowY = CARD_AREA.y() + cardCenterY;
    
    return QPoint(windowX, windowY);
}

std::vector<CardInfo> CardRecognizer::recognizeCards(const QImage& screenshot, const QStringList& targetCardTypes)
{
    std::vector<CardInfo> results;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // qDebug() << "识别目标卡片类型:" << targetCardTypes.size() << "种";

    try {
        // 获取卡片区域图像
        QImage cardAreaImage = screenshot.copy(CARD_AREA);
        // 使用颜色检测方法获取startY
        int startY = findStartYUsingColorDetection(cardAreaImage);
        
        // qDebug() << "找到分隔线，位置:" << startY;

        // 计算实际的处理区域
        int availableHeight = cardAreaImage.height() - startY;
        int processHeight = std::min(availableHeight, CARD_AREA_HEIGHT);
        
        // 截取处理区域的图像
        QImage cardsAreaImage = cardAreaImage.copy(0, startY, cardAreaImage.width(), processHeight);

        // 保存卡片区域图像
        // QString debugDir = getAppDataPath() + "/template_debug";
        // QDir().mkpath(debugDir);
        // cardsAreaImage.save(QString("%1/cards_area.png").arg(debugDir));
        
        // 计算实际可处理的行数
        int maxRows = processHeight / CARD_HEIGHT;

        // 遍历每个卡片位置
        for (int row = 0; row < std::min(TOTAL_ROWS, maxRows); ++row) {
            for (int col = 0; col < CARDS_PER_ROW; ++col) {
                // 计算当前卡片的位置
                int cardX = col * CARD_WIDTH;
                int cardY = row * CARD_HEIGHT;
                
                // 确保不会越界
                if (cardX + CARD_WIDTH > cardsAreaImage.width() || cardY + CARD_HEIGHT > cardsAreaImage.height()) {
                    continue;
                }

                // 计算ROI区域
                int roiX = cardX + CARD_TYPE_ROI_X;
                int roiY = cardY + CARD_TYPE_ROI_Y;
                
                if (roiX + CARD_TYPE_ROI_WIDTH > cardsAreaImage.width() || roiY + CARD_TYPE_ROI_HEIGHT > cardsAreaImage.height()) {
                    continue;
                }
                QRect roiRect(roiX, roiY, CARD_TYPE_ROI_WIDTH, CARD_TYPE_ROI_HEIGHT);

                try {
                    // 提取当前卡片的ROI区域
                    QImage cardRoi = cardsAreaImage.copy(roiRect);

                    // 计算当前ROI的哈希值
                    QString currentHash = calculateImageHash(cardRoi);

                    for (const QString& targetCard : targetCardTypes) {
                        if (cardTypeHashes.value(targetCard) == currentHash) {
                            QString matchedCard = targetCard;

                            //提取整个卡片用于星级和绑定状态识别
                            QImage singleCard = cardsAreaImage.copy(cardX, cardY, CARD_WIDTH, CARD_HEIGHT);

                            int cardLevel = recognizeCardLevel(singleCard);
                            bool isBound = recognizeCardBind(singleCard);
                            QPoint centerPos = calculateCardCenterPosition(row, col);
                            centerPos.setY(centerPos.y() + startY);
                            CardInfo cardInfo(matchedCard, cardLevel, isBound, centerPos, row, col);
                            results.push_back(cardInfo);
                            break;
                        }
                    }
                } catch (const std::exception& e) {
                    qDebug() << "处理卡片ROI时发生错误:" << e.what();
                }
            }
        }

    } catch (const std::exception& e) {
        qDebug() << "识别过程中发生错误:" << e.what();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    qDebug() << "卡片识别完成，用时:" << duration.count() << "ms，识别到" << results.size() << "张卡片";

    return results;
} 
