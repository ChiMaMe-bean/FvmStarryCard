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
#include "custombutton.h"
#include "utils.h"
#include "cardrecognizer.h"
#include "reciperecognizer.h"
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
    void performEnhancement();
    void addLog(const QString& message, LogType type = LogType::Info);
    int getDPIFromDC();
    void onCaptureAndRecognize();
    void onEnhancementConfigChanged();
    void saveEnhancementConfig();
    void loadEnhancementConfig();
    void loadEnhancementConfigFromFile();
    void onCardSettingButtonClicked();
    void onApplyMainCardToAll(int row, const QString& cardType, bool bind, bool unbound);
    void onApplySubCardToAll(int row, const QString& cardType, bool bind, bool unbound);
    void onMaxEnhancementLevelChanged();
    void onMinEnhancementLevelChanged();
    HWND GetHallWindow(HWND hWnd);
    bool IsGameWindowVisible(HWND hWnd);
    void clickRefresh();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int IsGameWindow(HWND hWndGame); 
    void setBackground(const QString &imagePath);
    void saveCustomBgPath();
    void loadCustomBgPath();
    void setupUI();
    void updateCurrentBgLabel();
    QImage captureGameWindow();
    QImage captureWindowByHandle(HWND hwnd, const QString& windowName = "");
    QImage captureImageRegion(const QImage& sourceImage, const QRect& rect, const QString& filename = "");
    void showRecognitionResults(const std::vector<CardInfo>& results);
    QWidget* createEnhancementConfigPage();
    int getMinSubCardLevel(int enhancementLevel);
    int getMaxSubCardLevel(int enhancementLevel);
    int getMaxEnhancementLevel() const;
    int getMinEnhancementLevel() const;
    QStringList getCardTypes() const;
    QStringList getRequiredCardTypesFromConfig();
    void saveCardSettingForRow(int row, const QString& mainCardType, const QString& subCardType,
                               bool mainBind, bool mainUnbound, bool subBind, bool subUnbound);
    
    // 四叶草识别相关方法
    bool loadCloverTemplates();
    bool loadBindStateTemplate();
    QString calculateImageHash(const QImage& image, const QRect& roi = QRect());
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
    
    // 翻页检测相关方法
    bool loadPageTemplates();
    bool isPageAtTop();
    bool isPageAtBottom();
    
    // 位置模板相关方法
    void loadPositionTemplates();

    // 配方识别相关方法（已迁移到 RecipeRecognizer）
    QStringList getAvailableRecipeTypes() const; // 获取所有可用的配方类型
    
    // 鼠标点击相关方法
    BOOL leftClickDPI(HWND hwnd, int x, int y);
    BOOL leftClick(HWND hwnd, int x, int y);
    BOOL closeHealthTip();
    
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
    void refreshGameWindow();

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
    int currentEnhancementLevel = 0; // 当前强化等级 (0表示未开始)
    HWND hwndGame = nullptr; // 游戏窗口
    HWND hwndHall = nullptr;   // 大厅窗口
    HWND hwndServer = nullptr; // 选服窗口
    
    // 全局位图数据
    COLORREF* globalPixelData = nullptr;
    int globalBitmapWidth = 0;
    int globalBitmapHeight = 0;

    QString defaultBgPath = ":/items/background/default.png";
    QString customBgPath = "";
    QString jsonPath = "custom_bg_config.json";
    QString enhancementConfigPath = "enhancement_config.json";
    CardRecognizer* cardRecognizer;
    QStringList requiredCardTypesForEnhancement; // 强化流程中使用的卡片类型列表
    
    // 四叶草识别相关数据
    QHash<QString, QString> cloverTemplateHashes; // 四叶草类型名 -> 哈希值（保留备用）
    QHash<QString, QImage> cloverTemplateImages; // 四叶草类型名 -> 模板图像
    QHash<QString, QVector<double>> cloverTemplateHistograms; // 四叶草类型名 -> 颜色直方图
    QImage bindStateTemplate; // 绑定状态模板图像
    QString bindStateTemplateHash; // 绑定状态模板哈希值
    bool cloverTemplatesLoaded = false;
    
    // 香料识别相关数据
    QHash<QString, QString> spiceTemplateHashes; // 香料类型名 -> 哈希值
    QHash<QString, QImage> spiceTemplateImages; // 香料类型名 -> 模板图像
    QHash<QString, QVector<double>> spiceTemplateHistograms; // 香料类型名 -> 颜色直方图
    bool spiceTemplatesLoaded = false;
    
    // 翻页检测相关数据
    QImage pageUpTemplate;      // 翻页到顶部模板图像
    QImage pageDownTemplate;    // 翻页到底部模板图像
    QVector<double> pageUpHistogram;    // 翻页到顶部颜色直方图
    QVector<double> pageDownHistogram;  // 翻页到底部颜色直方图
    bool pageTemplatesLoaded = false;

    // 位置模板相关数据
    QHash<QString, QString> positionTemplateHashes; // 位置模板名称 -> 哈希值

    RecipeRecognizer* recipeRecognizer; // 新增成员变量

    void updateRecipeCombo(); // 更新配方选择下拉框
};
#endif // STARRYCARD_H
