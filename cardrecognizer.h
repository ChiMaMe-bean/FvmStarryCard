#ifndef CARDRECOGNIZER_H
#define CARDRECOGNIZER_H

#include <QObject>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <QMap>
#include <QFileInfo>
#include <QPoint>
#include <QVector>

// 卡片信息结构体
struct CardInfo {
    std::string name;           // 卡片名称
    int level;                  // 卡片星级
    bool isBound;               // 是否绑定
    QPoint centerPosition;      // 卡片中心位置（相对于游戏窗口）
    int row;                    // 卡片在背包中的行位置
    int col;                    // 卡片在背包中的列位置
    
    CardInfo() : level(0), isBound(false), row(-1), col(-1) {}
    CardInfo(const std::string& cardName, int cardLevel, bool bound, QPoint center, int r, int c)
        : name(cardName), level(cardLevel), isBound(bound), centerPosition(center), row(r), col(c) {}
};

// 卡片模板管理类
class CardTemplateManager {
public:
    // 添加新的卡片模板
    void addTemplate(const std::string& cardName, const cv::Mat& templateImage);
    // 获取特定卡片的模板
    cv::Mat getTemplate(const std::string& cardName) const;
    // 获取所有已注册的卡片名称
    std::vector<std::string> getRegisteredCards() const;
    // 计算图像的特征哈希
    static cv::Mat computeHash(const cv::Mat& img);
    // 比较两个哈希值的相似度
    static double compareHash(const cv::Mat& hash1, const cv::Mat& hash2);

private:
    std::unordered_map<std::string, cv::Mat> templates;
    std::unordered_map<std::string, cv::Mat> templateHashes;
};

class CardRecognizer : public QObject
{
    Q_OBJECT

public:
    explicit CardRecognizer(QObject *parent = nullptr);
    bool loadTemplates();
    std::vector<CardInfo> recognizeCardsDetailed(const QImage& screenshot, const QStringList& targetCardTypes);
    std::vector<std::string> getRegisteredCards() const;
    
    // Card ROI constants
    static constexpr int CARD_TYPE_ROI_X = 8;
    static constexpr int CARD_TYPE_ROI_Y = 22;
    static constexpr int CARD_TYPE_ROI_WIDTH = 33;
    static constexpr int CARD_TYPE_ROI_HEIGHT = 15;
    
    static constexpr int CARD_LEVEL_ROI_X = 9;
    static constexpr int CARD_LEVEL_ROI_Y = 8;
    static constexpr int CARD_LEVEL_ROI_WIDTH = 6;
    static constexpr int CARD_LEVEL_ROI_HEIGHT = 8;
    
    static constexpr int CARD_BIND_ROI_X = 5;
    static constexpr int CARD_BIND_ROI_Y = 45;
    static constexpr int CARD_BIND_ROI_WIDTH = 6;
    static constexpr int CARD_BIND_ROI_HEIGHT = 7;

private:
    cv::Mat qImageToCvMat(const QImage& image);
    QImage cvMatToQImage(const cv::Mat& mat);
    std::vector<cv::Mat> cardTemplates;
    std::vector<std::string> templateNames;
    cv::Mat separatorLine;
    CardTemplateManager templateManager;
    
    const int CARD_WIDTH = 49;
    const int CARD_HEIGHT = 57;
    const cv::Rect CARD_AREA{559, 91, 343, 456};
    const double MATCH_THRESHOLD = 0.28;
    const int MAX_SEPARATOR_SEARCH_HEIGHT = 70;

    const int CARDS_PER_ROW = 7;
    const int TOTAL_ROWS = 7;
    const int CARD_AREA_HEIGHT = 399; // 7 * 57

    QMap<QString, QImage> m_levelTemplates;
    QImage m_bindTemplate;
    int recognizeCardLevel(const cv::Mat& cardMat);
    bool recognizeCardBind(const cv::Mat& cardMat);
    void loadLevelTemplates();
    void loadBindTemplate();
    QPoint calculateCardCenterPosition(int row, int col) const;
};

#endif // CARDRECOGNIZER_H 