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
#include "custombutton.h"
#include "utils.h"
#include "cardrecognizer.h"

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
    int SetDPIAware();
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
    void showRecognitionResults(const std::vector<std::string>& results);
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
    
    // 翻页检测相关方法
    bool loadPageTemplates();
    bool isPageAtTop();
    bool isPageAtBottom();

    QStackedWidget *centerStack = nullptr;
    QButtonGroup *buttonGroup = nullptr;
    QComboBox *themeCombo = nullptr;
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
    HWND targetWindow = nullptr;

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
    
    // 翻页检测相关数据
    QImage pageUpTemplate;      // 翻页到顶部模板图像
    QImage pageDownTemplate;    // 翻页到底部模板图像
    QVector<double> pageUpHistogram;    // 翻页到顶部颜色直方图
    QVector<double> pageDownHistogram;  // 翻页到底部颜色直方图
    bool pageTemplatesLoaded = false;
};
#endif // STARRYCARD_H
