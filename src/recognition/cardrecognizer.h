#ifndef CARDRECOGNIZER_H
#define CARDRECOGNIZER_H

#include <QObject>
#include <QImage>
#include <vector>
#include <string>
#include <QMap>
#include <QFileInfo>
#include <QPoint>
#include <QVector>

// 卡片信息结构体
struct CardInfo {
    QString name;               // 卡片名称
    int level;                  // 卡片星级
    bool isBound;               // 是否绑定
    QPoint centerPosition;      // 卡片中心位置（相对于游戏窗口）
    int row;                    // 卡片在背包中的行位置
    int col;                    // 卡片在背包中的列位置
    
    CardInfo() : level(0), isBound(false), row(-1), col(-1) {}
    CardInfo(const QString& cardName, int cardLevel, bool bound, QPoint center, int r, int c)
        : name(cardName), level(cardLevel), isBound(bound), centerPosition(center), row(r), col(c) {}
};

class CardRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit CardRecognizer(QObject *parent = nullptr);
    bool loadTemplates();
    QVector<CardInfo> recognizeCards(const QImage& screenshot, const QStringList& targetCardTypes);
    QStringList getRegisteredCards() const;
    
    // 获取模板哈希值的公开方法
    QString getCardTypeHash(const QString& cardName) const;
    QString getCardLevelHash(int level) const;  // level: 1-16
    QString getCardBindHash() const;
    
    // Card ROI constants
    static constexpr int CARD_TYPE_ROI_X = 8;
    static constexpr int CARD_TYPE_ROI_Y = 22;
    static constexpr int CARD_TYPE_ROI_WIDTH = 32;
    static constexpr int CARD_TYPE_ROI_HEIGHT = 16;
    
    static constexpr int CARD_LEVEL_ROI_X = 9;
    static constexpr int CARD_LEVEL_ROI_Y = 8;
    static constexpr int CARD_LEVEL_ROI_WIDTH = 6;
    static constexpr int CARD_LEVEL_ROI_HEIGHT = 8;
    
    static constexpr int CARD_BIND_ROI_X = 5;
    static constexpr int CARD_BIND_ROI_Y = 45;
    static constexpr int CARD_BIND_ROI_WIDTH = 6;
    static constexpr int CARD_BIND_ROI_HEIGHT = 7;

private:
    
    const int CARD_WIDTH = 49;
    const int CARD_HEIGHT = 57;
    const QRect CARD_AREA{559, 91, 343, 456};
    const double MATCH_THRESHOLD = 0.28;
    const int MAX_SEPARATOR_SEARCH_HEIGHT = 70;

    const int CARDS_PER_ROW = 7;
    const int TOTAL_ROWS = 7;
    const int CARD_AREA_HEIGHT = 399; // 7 * 57

    QMap<QString, QImage> m_levelTemplates;
    QImage m_bindTemplate;
    int recognizeCardLevel(QImage cardArea);
    bool recognizeCardBind(QImage cardArea);
    int findStartYUsingColorDetection(const QImage& screenshot);
    void loadLevelTemplates();
    void loadBindTemplate();
    QPoint calculateCardCenterPosition(int row, int col) const;

    // 卡片类型哈希值
    QHash<QString, QString> cardTypeHashes;
    QRect CARD_TYPE_ROI{8,22,32,16};

    // 卡片等级哈希值
    QStringList cardLevelHashes;
    void loadCardLevelHashes();
    const QRect CARD_LEVEL_ROI{9,8,6,8};

    // 绑定状态哈希值
    QString cardBindHash;
    void loadCardBindHashes();
    const QRect CARD_BOUND_ROI{5,45,6,7};

    // 计算图像哈希值
    QString calculateImageHash(const QImage& image, const QRect& roi = QRect());
};

#endif // CARDRECOGNIZER_H 