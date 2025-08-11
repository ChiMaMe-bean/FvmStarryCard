#ifndef STARRYCARD_H
#define STARRYCARD_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QTimer>
#include <QLabel>
#include <QTextEdit>
#include <QFileInfo>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileDialog>
#include <QCheckBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QScreen>
#include <QApplication>
#include <QSet>
#include <QSpinBox>
#include <QHash>
#include <QPixmap>
#include <QThread>
#include <QCryptographicHash>
#include <QColor>
#include <QVector>
#include <QList>
#include <QPair>
#include <QMutex>
#include <QWaitCondition>
#include <algorithm>
#include "../ui/custombutton.h"
#include "utils.h"
#include "../recognition/cardrecognizer.h"
#include "../recognition/reciperecognizer.h"
#include <windows.h>
#include <winuser.h>

QT_BEGIN_NAMESPACE
namespace Ui { class StarryCard; }
QT_END_NAMESPACE

// 日志类型枚举
enum class LogType {
    Info,       // 普通信息（黑色）
    Success,    // 成功信息（绿色）
    Warning,    // 警告信息（橙色）
    Error       // 错误信息（红色）
};

// 前置声明
class StarryCard;

// 强化工作线程类
class EnhancementWorker : public QObject
{
    Q_OBJECT

public:
    explicit EnhancementWorker(StarryCard* parent);
    void setParent(StarryCard* parent) { m_parent = parent; }

public slots:
    void startEnhancement();

signals:
    void logMessage(const QString& message, LogType type);
    void showWarningMessage(const QString& title, const QString& message);
    void enhancementFinished();
    void stopEnhancementRequested();

private:
    StarryCard* m_parent;
    void performEnhancement();
    BOOL performEnhancementOnce(const QVector<CardInfo>& cardVector);
    BOOL performCardProduce(const QVector<CardInfo>& cardVector);
    BOOL performCardProduceOnce();
    void getCardNeedProduce(); // 分析强化配置提取制卡需求
    void threadSafeSleep(int ms);
};

// 全局强化配置数据结构
struct GlobalEnhancementConfig {
    // 基本等级设置
    int maxLevel = 14;
    int minLevel = 1;
    
    // 每个等级的详细配置 (键格式: "等级-下一等级", 如 "0-1", "1-2")
    struct LevelConfig {
        // 副卡配置
        int subcard1 = 0;           // 副卡1星级 (-1表示无)
        int subcard2 = -1;          // 副卡2星级 (-1表示无)  
        int subcard3 = -1;          // 副卡3星级 (-1表示无)
        
        // 四叶草配置
        QString clover = "无";       // 四叶草类型
        bool cloverBound = false;    // 四叶草绑定状态
        bool cloverUnbound = false;  // 四叶草不绑状态
        
        // 主卡/副卡类型配置
        QString mainCardType = "无";   // 主卡类型
        bool mainCardBound = false;    // 主卡绑定状态
        bool mainCardUnbound = false;  // 主卡不绑状态
        QString subCardType = "无";    // 副卡类型
        bool subCardBound = false;     // 副卡绑定状态
        bool subCardUnbound = false;   // 副卡不绑状态
    };
    
    QMap<QString, LevelConfig> levelConfigs;  // 存储各等级配置
    
    // 辅助方法
    LevelConfig getLevelConfig(int fromLevel, int toLevel) const {
        QString key = QString("%1-%2").arg(fromLevel).arg(toLevel);
        return levelConfigs.value(key, LevelConfig());
    }
    
    bool hasLevelConfig(int fromLevel, int toLevel) const {
        QString key = QString("%1-%2").arg(fromLevel).arg(toLevel);
        return levelConfigs.contains(key);
    }
    
    void setLevelConfig(int fromLevel, int toLevel, const LevelConfig& config) {
        QString key = QString("%1-%2").arg(fromLevel).arg(toLevel);
        levelConfigs[key] = config;
    }
};

// 全局香料配置数据结构
struct GlobalSpiceConfig {
    struct SpiceItem {
        QString name;           // 香料名称
        bool used = true;       // 是否使用
        bool bound = false;     // 是否绑定
        QString limitType = "无限制";  // 限制类型
        int limitAmount = 0;    // 限制数量
        int spiceLevel = 0;     // 香料等级
    };
    
    QVector<SpiceItem> spices;  // 香料列表
    
    // 获取已启用的香料名称列表
    QStringList getUsedSpiceNames() const {
        QStringList result;
        for (const auto& spice : spices) {
            if (spice.used) {
                result.append(spice.name);
            }
        }
        return result;
    }
    
    // 根据名称查找香料配置
    SpiceItem* findSpiceByName(const QString& name) {
        for (auto& spice : spices) {
            if (spice.name == name) {
                return &spice;
            }
        }
        return nullptr;
    }
    
    // 根据等级查找香料配置
    SpiceItem* findSpiceByLevel(int level) {
        for (auto& spice : spices) {
            if (spice.spiceLevel == level && spice.used) {
                return &spice;
            }
        }
        return nullptr;
    }
    
    // 获取已启用的香料，按等级排序（从低到高）
    QVector<SpiceItem> getUsedSpicesByLevel() const {
        QVector<SpiceItem> result;
        for (const auto& spice : spices) {
            if (spice.used) {
                result.append(spice);
            }
        }
        // 按等级排序
        std::sort(result.begin(), result.end(), [](const SpiceItem& a, const SpiceItem& b) {
            return a.spiceLevel < b.spiceLevel;
        });
        return result;
    }
    
    // 获取指定等级范围内的已启用香料
    QVector<SpiceItem> getUsedSpicesInLevelRange(int minLevel, int maxLevel) const {
        QVector<SpiceItem> result;
        for (const auto& spice : spices) {
            if (spice.used && spice.spiceLevel >= minLevel && spice.spiceLevel <= maxLevel) {
                result.append(spice);
            }
        }
        // 按等级排序
        std::sort(result.begin(), result.end(), [](const SpiceItem& a, const SpiceItem& b) {
            return a.spiceLevel < b.spiceLevel;
        });
        return result;
    }
    
    // 清空配置
    void clear() {
        spices.clear();
    }
};

// 需要制作的卡片配置数据结构
struct CardProduceConfig {
    struct ProduceItem {
        QString cardType;       // 卡片类型
        int targetLevel;        // 目标等级
        bool bound;             // 绑定状态
        bool unbound;           // 不绑状态
        
        ProduceItem() : targetLevel(0), bound(false), unbound(false) {}
        ProduceItem(const QString& type, int level, bool b, bool ub)
            : cardType(type), targetLevel(level), bound(b), unbound(ub) {}
        
        // 比较操作符，用于去重
        bool operator==(const ProduceItem& other) const {
            return cardType == other.cardType && 
                   targetLevel == other.targetLevel && 
                   bound == other.bound && 
                   unbound == other.unbound;
        }
        
        // 小于操作符，用于排序
        bool operator<(const ProduceItem& other) const {
            if (cardType != other.cardType) return cardType < other.cardType;
            if (targetLevel != other.targetLevel) return targetLevel < other.targetLevel;
            if (bound != other.bound) return bound < other.bound;
            return unbound < other.unbound;
        }
    };
    
    QVector<ProduceItem> produceItems;  // 需要制作的卡片列表
    
    // 添加制作项目（自动去重）
    void addProduceItem(const QString& cardType, int level, bool bound, bool unbound) {
        ProduceItem newItem(cardType, level, bound, unbound);
        // 检查是否已存在相同的项目
        for (const auto& existingItem : produceItems) {
            if (existingItem == newItem) {
                return; // 已存在，不添加
            }
        }
        produceItems.append(newItem);
    }
    
    // 手动去重方法
    void removeDuplicates() {
        QVector<ProduceItem> uniqueItems;
        for (const auto& item : produceItems) {
            bool exists = false;
            for (const auto& uniqueItem : uniqueItems) {
                if (item == uniqueItem) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                uniqueItems.append(item);
            }
        }
        produceItems = uniqueItems;
    }
    
    // 排序制作项目（按类型和等级从高到低排序）
    void sortProduceItems() {
        std::sort(produceItems.begin(), produceItems.end(), [](const ProduceItem& a, const ProduceItem& b) {
            // 首先按卡片类型排序
            if (a.cardType != b.cardType) return a.cardType < b.cardType;
            // 然后按等级从高到低排序
            if (a.targetLevel != b.targetLevel) return a.targetLevel > b.targetLevel;
            // 最后按绑定状态排序
            if (a.bound != b.bound) return a.bound < b.bound;
            return a.unbound < b.unbound;
        });
    }
    
    // 获取指定卡片类型和等级的制作项目
    QVector<ProduceItem> getProduceItemsByTypeAndLevel(const QString& cardType, int level) const {
        QVector<ProduceItem> result;
        for (const auto& item : produceItems) {
            if (item.cardType == cardType && item.targetLevel == level) {
                result.append(item);
            }
        }
        return result;
    }
    
    // 获取指定卡片类型的所有制作项目
    QVector<ProduceItem> getProduceItemsByType(const QString& cardType) const {
        QVector<ProduceItem> result;
        for (const auto& item : produceItems) {
            if (item.cardType == cardType) {
                result.append(item);
            }
        }
        return result;
    }
    
    // 清空配置
    void clear() {
        produceItems.clear();
    }
    
    // 获取所有不同的卡片类型
    QStringList getUniqueCardTypes() const {
        QStringList types;
        for (const auto& item : produceItems) {
            if (!types.contains(item.cardType)) {
                types.append(item.cardType);
            }
        }
        return types;
    }
};

// 全局配置数据实例声明
extern GlobalEnhancementConfig g_enhancementConfig;
extern GlobalSpiceConfig g_spiceConfig;
extern CardProduceConfig g_cardProduceConfig;

// 制卡统计数据结构
struct ProductionStatistics {
    QMap<QString, int> spiceStats;    // 香料使用统计：香料名称 -> 使用次数
    QMap<QString, int> recipeStats;   // 配方使用统计：配方名称 -> 使用次数
    
    // 添加香料使用记录
    void addSpiceUsage(const QString& spiceName, int count = 5) {
        if (spiceStats.contains(spiceName)) {
            spiceStats[spiceName] += count;
        } else {
            spiceStats[spiceName] = count;
        }
    }
    
    // 添加配方使用记录
    void addRecipeUsage(const QString& recipeName, int count = 1) {
        if (recipeStats.contains(recipeName)) {
            recipeStats[recipeName] += count;
        } else {
            recipeStats[recipeName] = count;
        }
    }
    
    // 获取香料总使用次数
    int getTotalSpiceUsage() const {
        int total = 0;
        for (auto it = spiceStats.begin(); it != spiceStats.end(); ++it) {
            total += it.value();
        }
        return total;
    }
    
    // 获取配方总使用次数
    int getTotalRecipeUsage() const {
        int total = 0;
        for (auto it = recipeStats.begin(); it != recipeStats.end(); ++it) {
            total += it.value();
        }
        return total;
    }
    
    // 清空统计数据
    void clear() {
        spiceStats.clear();
        recipeStats.clear();
    }
};

extern ProductionStatistics g_productionStats;

// 卡片设置对话框
class CardSettingDialog : public QDialog
{
    Q_OBJECT

public:
    CardSettingDialog(int row, const QStringList& cardTypes, QWidget *parent = nullptr);
    
    // 获取设置值
    QString getMainCardType() const;
    QString getSubCardType() const;
    bool getMainCardBind() const;
    bool getMainCardUnbound() const;
    bool getSubCardBind() const;
    bool getSubCardUnbound() const;
    
    // 设置值
    void setMainCardType(const QString& type);
    void setSubCardType(const QString& type);
    void setMainCardBind(bool bind, bool unbound);
    void setSubCardBind(bool bind, bool unbound);

private slots:
    void onApplyMainCardToAll();
    void onApplySubCardToAll();
    void onConfigChanged();

signals:
    void applyMainCardToAll(int row, const QString& cardType, bool bind, bool unbound);
    void applySubCardToAll(int row, const QString& cardType, bool bind, bool unbound);

private:
    int m_row;
    QComboBox* m_mainCardCombo;
    QComboBox* m_subCardCombo;
    QCheckBox* m_mainBindCheck;
    QCheckBox* m_mainUnboundCheck;
    QCheckBox* m_subBindCheck;
    QCheckBox* m_subUnboundCheck;
    QPushButton* m_applyMainBtn;
    QPushButton* m_applySubBtn;
    bool m_configChanged;
};

class StarryCard : public QMainWindow
{
    Q_OBJECT

public:
    StarryCard(QWidget *parent = nullptr);
    ~StarryCard();
    
    // 任务延时
    void sleepByQElapsedTimer(int ms);
    
    // 声明EnhancementWorker为友元类
    friend class EnhancementWorker;

private slots:
    void changeBackground(const QString &theme);
    void selectCustomBackground();
    void updateSelectBtnState(const QString &theme);
    void startMouseTracking();
    void stopMouseTracking();
    void trackMousePosition(QPoint pos);
    void updateHandleDisplay(HWND hwnd);
    void handleButtonRelease();
    void startEnhancement();
    void stopEnhancement();
    void addLog(const QString& message, LogType type = LogType::Info);
    void showWarningMessage(const QString& title, const QString& message);
    void onEnhancementFinished();
    int getDPIFromDC();
    void onCaptureAndRecognize();
    void onEnhancementConfigChanged();
    void saveEnhancementConfig();
    void scheduleConfigSave(); // 计划延迟保存配置
    void loadEnhancementConfig();
    void loadEnhancementConfigFromFile();
    void onCardSettingButtonClicked();
    void onApplyMainCardToAll(int row, const QString& cardType, bool bind, bool unbound);
    void onApplySubCardToAll(int row, const QString& cardType, bool bind, bool unbound);
    void onMaxEnhancementLevelChanged();
    void onMinEnhancementLevelChanged();
    void updateEnhancementTableUI(); // 更新强化表格UI状态
    void clickRefresh();

signals:
    void startEnhancementSignal();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setBackground(const QString &imagePath);
    void saveCustomBgPath();
    void loadCustomBgPath();
    void setupUI();
    void updateCurrentBgLabel();
    QImage captureWindowByHandle(HWND hwnd, const QString& windowName = "");
    QImage captureImageRegion(const QImage& sourceImage, const QRect& rect, const QString& filename = "");
    void showRecognitionResults(const QVector<CardInfo>& results);
    QWidget* createEnhancementConfigPage();
    QWidget* createSpiceConfigPage();
    int getMinSubCardLevel(int enhancementLevel);
    int getMaxSubCardLevel(int enhancementLevel);
    int getMaxEnhancementLevel() const;
    int getMinEnhancementLevel() const;
    QStringList getCardTypes() const;
    QStringList getRequiredCardTypesFromConfig();
    void saveCardSettingForRow(int row, const QString& mainCardType, const QString& subCardType,
                               bool mainBind, bool mainUnbound, bool subBind, bool subUnbound);
    
    // 全局配置相关方法
    bool loadGlobalEnhancementConfig();
    
    // 四叶草识别相关方法
    bool loadCloverTemplates();
    bool loadBindStateTemplate();
    QString calculateImageHash(const QImage& image, const QRect& roi = QRect());
    double calculateHashSimilarity(const QString& hash1, const QString& hash2); // 新增哈希相似度计算方法
    double calculateColorHistogramSimilarity(const QImage& image1, const QImage& image2, const QRect& roi = QRect());
    QVector<double> calculateColorHistogram(const QImage& image, const QRect& roi = QRect());
    bool isCloverBound(const QImage& cloverImage);
    QPair<bool, bool> recognizeClover(const QString& cloverType, bool clover_bound, bool clover_unbound);
    
    // 辅助方法
    bool checkCloverBindState(const QImage& cloverImage, bool clover_bound, bool clover_unbound, bool& actualBindState);
    bool recognizeSingleClover(const QImage& cloverImage, const QString& cloverType, int positionX, int positionY, 
                               bool clover_bound, bool clover_unbound);
    
    // 香料识别相关方法
    bool loadSpiceTemplates();
    QPair<bool, bool> recognizeSpice(const QString& spiceType, bool spice_bound, bool spice_unbound);
    bool recognizeSingleSpice(const QImage& spiceImage, const QString& spiceType, int positionX, int positionY, 
                              bool spice_bound, bool spice_unbound);
    bool checkSpiceBindState(const QImage& spiceImage, bool spice_bound, bool spice_unbound, bool& actualBindState);
    bool isSpiceBound(const QImage& spiceImage);
    QPair<bool, bool> getSpiceBindingConfig(const QString& spiceType) const;
    
    // 香料配置相关方法
    QList<QPair<QString, int>> calculateSpiceAllocation(int totalCardCount);
    
    // 翻页检测相关方法
    bool loadPageTemplates();
    bool isPageAtTop();
    bool isPageAtBottom();
    
    // 制卡相关方法
    QStringList getSelectedSpices(); // 获取勾选的香料列表，按优先级排序
    bool recognizeMakeButton(); // 识别制作按钮
    void performCardMaking(); // 执行制卡流程
    bool verifyRecipeTemplate(const QString& targetRecipe); // 验证配方模板匹配

    // 配方识别相关方法（已迁移到 RecipeRecognizer）
    QStringList getAvailableRecipeTypes() const; // 获取所有可用的配方类型
    
    // 鼠标点击相关方法
    BOOL leftClickDPI(HWND hwnd, int x, int y);
    BOOL leftClick(HWND hwnd, int x, int y);
    BOOL closeHealthTip(uint8_t retryCount = 10);
    
    // 窗口相关方法
    HWND GetHallWindow(HWND hWnd);
    bool IsGameWindowVisible(HWND hWnd);
    
    // 窗口位图获取方法
    BOOL getWindowBitmap(HWND hwnd, int& width, int& height, COLORREF*& pixelData);
    
    // 颜色识别相关方法
    BOOL isGamePlatformColor(COLORREF color, int platformType);
    int recognizeBitmapRegionColor(int platformType, COLORREF *pHallShot[4320], const QRect& region);

    // 图像哈希对比
    // uint64_t calculateImageHash(const QImage& image, const QRect& roi);
    int hammingDistance(uint64_t hash1, uint64_t hash2);
    int matchImages(const QString& path1, uint64_t hash);

    // 寻找游戏窗口相关方法
    HWND getGameWindow(HWND hwndHall);
    HWND getActiveGameWindow(HWND hwndHall);
    // 寻找选服窗口相关方法
    HWND getServerWindow(HWND hwndChild);
    HWND getActiveServerWindow(HWND hwndHall);
    int waitServerInWindow(RECT rectServer, int *px = nullptr, int *py = nullptr);
    int findLatestServer(int platformType, int *px, int *py);
    BOOL refreshGameWindow();

    QStackedWidget *centerStack = nullptr;
    QButtonGroup *buttonGroup = nullptr;
    QComboBox *themeCombo = nullptr;
    QComboBox *recipeCombo = nullptr;
    QComboBox *debugCombo = nullptr;
    QPushButton *selectCustomBgBtn = nullptr;
    CustomButton *trackMouseBtn = nullptr;
    QLineEdit *handleDisplayEdit = nullptr;
    QLabel *windowTitleLabel = nullptr;
    QLabel *currentBgLabel = nullptr;
    QPushButton *enhancementBtn = nullptr;
    QPushButton *captureBtn = nullptr;
    QTimer *enhancementTimer = nullptr;
    QTextEdit *logTextEdit = nullptr;
    QTableWidget *enhancementTable = nullptr;
    QSpinBox *maxEnhancementLevelSpinBox = nullptr;
    QSpinBox *minEnhancementLevelSpinBox = nullptr;
    
    bool isTracking = false;
    bool isEnhancing = false;
    HWND hwndGame = nullptr; // 游戏窗口
    HWND hwndHall = nullptr;   // 大厅窗口
    HWND hwndServer = nullptr; // 选服窗口
    
    // 全局位图数据
    COLORREF* globalPixelData = nullptr;
    int globalBitmapWidth = 0;
    int globalBitmapHeight = 0;

    QString defaultBgPath = ":/images/background/default.png";
    QString customBgPath = "";
    QString jsonPath = "custom_bg_config.json";
    QString enhancementConfigPath = "enhancement_config.json";
    CardRecognizer* cardRecognizer;
    QStringList requiredCardTypes; // 强化流程中使用的卡片类型列表
    
    // 四叶草识别相关数据
    QHash<QString, QString> cloverTemplateHashes; // 四叶草类型名 -> 哈希值（保留备用）
    QHash<QString, QImage> cloverTemplateImages; // 四叶草类型名 -> 模板图像
    QHash<QString, QVector<double>> cloverTemplateHistograms; // 四叶草类型名 -> 颜色直方图
    QImage bindStateTemplate; // 绑定状态模板图像
    QString bindStateTemplateHash; // 绑定状态模板哈希值
    bool cloverTemplatesLoaded = false;
    
    // 香料识别相关数据
    QHash<QString, QString> spiceTemplateHashes; // 香料类型名 -> 哈希值
    bool spiceTemplatesLoaded = false;
    QRect spiceTemplateRoi = QRect(6, 6, 32, 16); // 香料模板ROI区域
    
    // 全局香料类型定义 - 避免多处重复定义导致的不一致
    static const QStringList getSpiceTypes() {
        return QStringList{
            "天然香料", "上等香料", "秘制香料", "极品香料", "皇室香料",
            "魔幻香料", "精灵香料", "天使香料", "圣灵香料"
        };
    }
    BOOL checkSpicePosState(QImage screenshot, const QRect& pos, const QString& templateName);
    const QRect SPICE_AREA_HOUSE = QRect(157, 372, 49, 49); // 合成屋香料区域
    
    // 翻页检测相关数据
    QImage pageUpTemplate;      // 翻页到顶部模板图像
    QImage pageDownTemplate;    // 翻页到底部模板图像
    QVector<double> pageUpHistogram;    // 翻页到顶部颜色直方图
    QVector<double> pageDownHistogram;  // 翻页到底部颜色直方图
    bool pageTemplatesLoaded = false;

    // 位置模板相关方法
    void loadPositionTemplates();
    QString recognizeCurrentPosition(QImage screenshot);

    // 加载合成屋内卡片位置模板
    void loadSynHousePosTemplates();
    QHash<QString, QString> synHousePosTemplateHashes; // 合成屋内卡片位置模板名称 -> 哈希值
    BOOL checkSynHousePosState(QImage screenshot, const QRect& pos, const QString& templateName);
    const QRect MAIN_CARD_POS = QRect(269, 332, 32, 32); // 主卡位置
    const QRect SUB_CARD_POS = QRect(269, 261, 32, 32);  // 副卡位置
    const QRect INSURANCE_POS = QRect(382, 423, 20, 20); // 保险位置
    const QRect PRODUCE_READY_POS = QRect(375, 364, 32, 32); // 制卡准备位置
    const QRect ENHANCE_BUTTON_POS = QRect(261, 425, 20, 20); // 强化按钮位置
    const QRect ENHANCE_SCROLL_BAR_BOTTOM = QRect(902, 526, 16, 16); // 强化滚动条底部位置

    // 页面跳转枚举
    enum class PageType {
        CardEnhance,    // 卡片强化页面
        CardProduce     // 卡片制作页面
    };

    BOOL goToPage(PageType targetPage, uint8_t retryCount = 12);
    
    // 滚动条相关方法
    void fastMouseDrag(int startX, int startY, int distance, bool downward = true);
    BOOL resetScrollBar();
    int getLengthOfScrollBar(QImage screenshot);
    int getPositionOfScrollBar(QImage screenshot);

    // 位置模板相关数据
    QHash<QString, QString> positionTemplateHashes; // 位置模板名称 -> 哈希值
    
    // 游戏界面位置常量
    static const QPoint CARD_ENHANCE_POS;       // 卡片强化按钮位置 (94,326)
    static const QPoint CARD_PRODUCE_POS;       // 卡片制作按钮位置 (94,260)
    static const QPoint SYNTHESIS_HOUSE_POS;    // 合成屋按钮位置 (675,556)
    static const QPoint RANKING_POS;            // 排行榜位置 (178,96)
    static const QPoint ENHANCE_SCROLL_TOP;     // 强化滚动条顶部位置 (902, 98)

    // 卡片强化属性
    int maxEnhancementLevel = 10;
    int minEnhancementLevel = 1;

    RecipeRecognizer* recipeRecognizer; // 新增成员变量
    
    // 线程相关
    QThread* enhancementThread;
    EnhancementWorker* enhancementWorker;
    
    // 延迟保存相关
    QTimer* configSaveTimer; // 配置保存延迟定时器
    QTimer* spiceSaveTimer; // 香料配置保存延迟定时器
    
    // 香料配置相关
    QTableWidget* spiceTable; // 香料配置表格
    
private slots:
    void onConfigSaveTimeout(); // 配置保存超时槽函数
    void onSpiceSaveTimeout(); // 香料配置保存超时槽函数
    void onSpiceConfigChanged(); // 香料配置改变槽函数
    void scheduleSpiceConfigSave(); // 计划延迟保存香料配置
    void loadSpiceConfig(); // 加载香料配置
    void saveSpiceConfig(); // 保存香料配置
    bool loadGlobalSpiceConfig(); // 加载全局香料配置（仅used=true的项目）
    
    // 统计相关方法
    void loadProductionStatistics(); // 加载制卡统计数据
    void saveProductionStatistics(); // 保存制卡统计数据
    void addProductionRecord(const QString& spiceName, const QString& recipeName); // 添加制卡记录（仅累计，不保存）
    
public:
    void updateRecipeCombo(); // 更新配方选择下拉框
};
#endif // STARRYCARD_H
