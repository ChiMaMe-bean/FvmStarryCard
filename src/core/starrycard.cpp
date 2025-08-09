#include "starrycard.h"
#include "../ui/custombutton.h"

// 临时调试函数声明
// void debugResources();
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QToolButton>
#include <QSizePolicy>
#include <QFileDialog>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPalette>
#include <QBrush>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTableWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QMouseEvent>
#include <QThread>
#include <QTime>
#include <QElapsedTimer>
#include <Windows.h>  // 包含 Windows.h 头文件，用于获取屏幕分辨率
#include <cstring>
#include <QTimer>
#include <QEventLoop>
#include <QMessageBox>
#include <QTextEdit>
#include <QScreen>
#include <QWindow>
#include <QGuiApplication>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QtMath>
#include <QScrollBar>
#include <QRegularExpression>
#include <algorithm>
#include <map>

// RGB颜色分量提取宏定义(rgb为0x00RRGGBB类型)
#define bgrRValue(rgb)      (LOBYTE((rgb)>>16))  // 红色分量
#define bgrGValue(rgb)      (LOBYTE(((WORD)(rgb)) >> 8))  // 绿色分量
#define bgrBValue(rgb)      (LOBYTE(rgb))  // 蓝色分量
// #include <chrono>

// 定义全局变量，用于存储 DPI 信息
int DPI = 96;  // 默认 DPI 值为 96，即 100% 缩  放

// 定义静态常量
const QPoint StarryCard::CARD_ENHANCE_POS(94, 326);      // 卡片强化按钮位置
const QPoint StarryCard::CARD_PRODUCE_POS(94, 260);      // 卡片制作按钮位置
const QPoint StarryCard::SYNTHESIS_HOUSE_POS(675, 556);  // 合成屋按钮位置
const QPoint StarryCard::ENHANCE_SCROLL_TOP(910, 120);    // 强化滚动条顶部位置

// 定义全局强化配置数据实例
GlobalEnhancementConfig g_enhancementConfig;
GlobalSpiceConfig g_spiceConfig;
CardProduceConfig g_cardProduceConfig;

StarryCard::StarryCard(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置DPI感知
    DPI = getDPIFromDC();

    // 设置窗口属性
    setWindowTitle("星空强卡器");
    setFixedSize(800, 600);
    
    // 设置窗口图标
    setWindowIcon(QIcon(":/images/icons/icon128.ico"));
    
    // 禁用最大化按钮，只保留最小化和关闭按钮
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    // 检查必要的目录和文件
    qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    qDebug() << "Current working directory:" << QDir::currentPath();
    
    // QStringList requiredDirs = {
    //     "items",
    //     "items/position",
    //     "items/card",
    //     "screenshots"
    // };

    // for (const QString& dir : requiredDirs) {
    //     QDir checkDir(dir);
    //     if (!checkDir.exists()) {
    //         if (!checkDir.mkpath(".")) {
    //             qDebug() << "Failed to create directory:" << dir;
    //         } else {
    //             qDebug() << "Created directory:" << dir;
    //         }
    //     } else {
    //         // qDebug() << "Directory exists:" << dir;
    //     }
    // }

    // 读取存储的自定义背景路径
    loadCustomBgPath();

    // 初始设置默认背景
    setBackground(defaultBgPath);
    
    // 初始化延迟保存定时器
    configSaveTimer = new QTimer(this);
    configSaveTimer->setSingleShot(true); // 只触发一次
    configSaveTimer->setInterval(800); // 800ms延迟，避免频繁保存
    connect(configSaveTimer, &QTimer::timeout, this, &StarryCard::onConfigSaveTimeout);
    
    // 初始化香料配置延迟保存定时器
    spiceSaveTimer = new QTimer(this);
    spiceSaveTimer->setSingleShot(true); // 只触发一次
    spiceSaveTimer->setInterval(800); // 800ms延迟，避免频繁保存
    connect(spiceSaveTimer, &QTimer::timeout, this, &StarryCard::onSpiceSaveTimeout);

    // 初始化UI
    setupUI();

    // // 临时调试：测试资源系统
    // debugResources();

    cardRecognizer = new CardRecognizer(this);
    
    // 初始化四叶草识别模板
    loadCloverTemplates();
    loadBindStateTemplate();
    loadPageTemplates();
    
    // 初始化香料识别模板
    loadSpiceTemplates();
    
    // 初始化位置模板
    loadPositionTemplates();

    // 初始化合成屋内卡片位置模板
    loadSynHousePosTemplates();
    
    // 初始化 RecipeRecognizer
    recipeRecognizer = new RecipeRecognizer();
    
    // 回调函数已移除，RecipeRecognizer现在直接实现这些功能
    
    // 初始化配方识别模板
    recipeRecognizer->loadRecipeTemplates();
    
    // 更新配方选择下拉框
    updateRecipeCombo();
    
    // 初始化线程
    enhancementThread = new QThread(this);
    enhancementWorker = new EnhancementWorker(this);
    enhancementWorker->moveToThread(enhancementThread);
    
    // 连接信号和槽 - 使用QueuedConnection确保线程安全
    connect(this, &StarryCard::startEnhancementSignal, enhancementWorker, &EnhancementWorker::startEnhancement, Qt::QueuedConnection);
    connect(enhancementWorker, &EnhancementWorker::logMessage, this, &StarryCard::addLog, Qt::QueuedConnection);
    connect(enhancementWorker, &EnhancementWorker::showWarningMessage, this, &StarryCard::showWarningMessage, Qt::QueuedConnection);
    connect(enhancementWorker, &EnhancementWorker::enhancementFinished, this, &StarryCard::onEnhancementFinished, Qt::QueuedConnection);
    
    // 启动线程
    enhancementThread->start();
    
    // 测试图像哈希算法
    qDebug() << "=== 测试图像哈希算法 ===";
    QImage testImage(8, 8, QImage::Format_RGB32);
    testImage.fill(Qt::white);
    QString testHash = calculateImageHash(testImage);
    qDebug() << "8x8白色图像哈希值:" << testHash;
    qDebug() << "=== 图像哈希算法测试结束 ===";

    addLog("欢迎使用星空强卡器", LogType::Info);
}

void StarryCard::setupUI()
{
    // 创建主容器
    QWidget *mainWidget = new QWidget(this);
    setCentralWidget(mainWidget);

    // 使用 QVBoxLayout 作为主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建水平布局，用于放置三个区域
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    // 区域一（左侧）
    QWidget *leftWidget = new QWidget();
    leftWidget->setStyleSheet("background-color: transparent;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setAlignment(Qt::AlignTop);
    leftLayout->setSpacing(0);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // 创建按钮组
    buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);

    // 添加四个功能按钮
    QToolButton *btn1 = new QToolButton();
    QToolButton *btn2 = new QToolButton();
    QToolButton *btn3 = new QToolButton();
    QToolButton *btn4 = new QToolButton();

    // 设置按钮样式
    QString buttonStyle = R"(
        QToolButton {
            background-color: rgba(240, 240, 240, 200);
            border: 1px solid rgba(240, 240, 240, 200);
            color: black;
            padding: 3px;
        }
        QToolButton:checked {
            background-color: rgba(33, 150, 243, 200);
            color: white;
        }
        QToolButton:!checked:hover {
            background-color: rgba(224, 224, 224, 200);
        }
    )";

    // 设置图标和文本布局
    QSize iconSize(32, 32);
    QList<QToolButton*> buttons = {btn1, btn2, btn3, btn4};
    QStringList buttonTexts = {"强化日志", "强化方案", "制卡方案", "强化统计"};
    QStringList iconPaths = {":/images/icons/强化日志.svg", ":/images/icons/强化方案.svg", ":/images/icons/制卡方案.svg", ":/images/icons/强化统计.svg"};
    
    for (int idx = 0; idx < buttons.size(); ++idx) {
        QToolButton *btn = buttons[idx];
        if (btn) {
            btn->setCheckable(true);
            btn->setStyleSheet(buttonStyle);
            
            // 创建自适应图标，支持选中/非选中状态切换
            QIcon adaptiveIcon;
            
            // 加载原始SVG，添加错误检查
            QIcon originalIcon(iconPaths[idx]);
            if (originalIcon.isNull() || originalIcon.pixmap(iconSize).isNull()) {
                qWarning() << "无法加载按钮图标:" << iconPaths[idx];
                // 使用默认图标作为fallback
                originalIcon = QIcon(":/images/icons/downArrow.svg");
            }
            
            // 非选中状态：创建轮廓效果（边框样式）
            QPixmap uncheckedPixmap = originalIcon.pixmap(iconSize);
            QPainter uncheckedPainter(&uncheckedPixmap);
            
            // 方法1：降低透明度以模拟边框效果
            uncheckedPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            uncheckedPainter.fillRect(uncheckedPixmap.rect(), QColor(0, 0, 0, 100)); // 更低的透明度
            uncheckedPainter.end();
            
            // 选中状态：使用原始图标
            QPixmap checkedPixmap = originalIcon.pixmap(iconSize);
            
            // 添加到自适应图标
            adaptiveIcon.addPixmap(uncheckedPixmap, QIcon::Normal, QIcon::Off);   // 非选中
            adaptiveIcon.addPixmap(checkedPixmap, QIcon::Normal, QIcon::On);      // 选中
            
            btn->setIcon(adaptiveIcon);
            btn->setIconSize(iconSize);
            btn->setText(buttonTexts[idx]);
            btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            leftLayout->addWidget(btn);
        }
    }

    // 将按钮加入按钮组
    buttonGroup->addButton(btn1, 0);
    buttonGroup->addButton(btn2, 1);
    buttonGroup->addButton(btn3, 2);
    buttonGroup->addButton(btn4, 3);

    // 区域二（中间）
    centerStack = new QStackedWidget();
    // centerStack->setStyleSheet("background-color: rgba(102, 204, 255, 60);");
    centerStack->setStyleSheet("background-color: transparent;");


    // 创建日志页面
    QWidget* logPage = new QWidget();
    QVBoxLayout* logLayout = new QVBoxLayout(logPage);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->setSpacing(0);
    
    logTextEdit = new QTextEdit();
    logTextEdit->setReadOnly(true);
    logTextEdit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    logTextEdit->setCursor(Qt::ArrowCursor);
    logTextEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: rgba(255, 255, 255, 160);
            font-family: "Microsoft YaHei";
            font-size: 10pt;
            margin: 0;
            padding: 5px;
            border: 2px solid rgba(102, 204, 255, 200);
            border-radius: 8px;
        }
    )");
    logLayout->addWidget(logTextEdit);

    // 创建强化方案配置页面
    QWidget *enhancementPage = createEnhancementConfigPage();
    
    // 创建第三个页面 - 香料配置页面
    QWidget *page3 = createSpiceConfigPage();

    // 创建第四个页面 - 与页面三风格保持一致
    QWidget *page4 = new QWidget();
    page4->setStyleSheet("background-color: transparent;");
    
    QVBoxLayout* page4Layout = new QVBoxLayout(page4);
    page4Layout->setContentsMargins(5, 5, 5, 5);
    page4Layout->setSpacing(10);
    
    // 添加标题
    QLabel* page4Title = new QLabel("高级功能配置");
    page4Title->setAlignment(Qt::AlignCenter);
    page4Title->setStyleSheet(R"(
        font-size: 16px;
        font-weight: bold; 
        color: #003D7A;
        background-color: rgba(125, 197, 255, 150);
        border-radius: 8px;
        padding: 8px;
    )");
    page4Layout->addWidget(page4Title);
    
    // 创建配置区域
    QWidget* configArea4 = new QWidget();
    configArea4->setStyleSheet(R"(
        QWidget {
            background-color: rgba(255, 255, 255, 160);
            border: 2px solid rgba(102, 204, 255, 200);
            border-radius: 8px;
        }
    )");
    
    QVBoxLayout* configLayout4 = new QVBoxLayout(configArea4);
    configLayout4->setContentsMargins(15, 15, 15, 15);
    configLayout4->setSpacing(10);
    
    // 添加一些示例配置选项
    QLabel* configLabel4 = new QLabel("此页面预留用于其他高级功能配置");
    configLabel4->setAlignment(Qt::AlignCenter);
    configLabel4->setStyleSheet(R"(
        color: #003D7A;
        font-size: 14px;
        padding: 20px;
    )");
    configLayout4->addWidget(configLabel4);
    
    // 添加一些占位按钮
    QHBoxLayout* buttonLayout4 = new QHBoxLayout();
    buttonLayout4->setSpacing(10);
    
    QPushButton* option1Btn4 = new QPushButton("选项1");
    option1Btn4->setFixedSize(120, 35);
    option1Btn4->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
    )");
    
    QPushButton* option2Btn4 = new QPushButton("选项2");
    option2Btn4->setFixedSize(120, 35);
    option2Btn4->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(125, 197, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(125, 197, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: rgba(125, 197, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
    )");
    
    buttonLayout4->addStretch();
    buttonLayout4->addWidget(option1Btn4);
    buttonLayout4->addWidget(option2Btn4);
    buttonLayout4->addStretch();
    
    configLayout4->addLayout(buttonLayout4);
    
    page4Layout->addWidget(configArea4, 1);
    page4Layout->addStretch();

    centerStack->addWidget(logPage);
    centerStack->addWidget(enhancementPage);
    centerStack->addWidget(page3);
    centerStack->addWidget(page4);

    // 默认选中"强化日志"按钮
    btn1->setChecked(true);
    centerStack->setCurrentIndex(0);

    // 连接按钮组的信号到页面切换
    connect(buttonGroup, &QButtonGroup::buttonClicked, this,
            [this](QAbstractButton* button) {
                centerStack->setCurrentIndex(buttonGroup->id(button));
            });

    // 区域三（右侧）
    QWidget *rightWidget = new QWidget();
    rightWidget->setStyleSheet("background-color: transparent;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->addStretch(1);

    // 创建垂直布局用于放置句柄相关控件
    QVBoxLayout *handleLayout = new QVBoxLayout();

    // 创建水平布局用于放置按钮和句柄显示
    QHBoxLayout *handleButtonLayout = new QHBoxLayout();

    // 添加追踪鼠标的按钮
    trackMouseBtn = new CustomButton(rightWidget);
    trackMouseBtn->setText("绑定句柄");
    trackMouseBtn->setFixedHeight(40);
    trackMouseBtn->setFixedWidth(80);
    trackMouseBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
    )");
    connect(trackMouseBtn, &QPushButton::pressed, this, &StarryCard::startMouseTracking);
    connect(trackMouseBtn, &CustomButton::leftButtonReleased, this, &StarryCard::handleButtonRelease);
    handleButtonLayout->addWidget(trackMouseBtn);

    // 添加文本框用于显示句柄
    handleDisplayEdit = new QLineEdit(rightWidget);
    handleDisplayEdit->setReadOnly(true);
    handleDisplayEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: rgba(255, 255, 255, 220);
            border: 2px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 1px 1px;
            color: #003D7A;
            font-size: 12px;
        }
    )");
    QFont font = handleDisplayEdit->font();
    font.setPointSize(10);
    handleDisplayEdit->setFont(font);
    handleButtonLayout->addWidget(handleDisplayEdit);

    // 添加标题显示
    windowTitleLabel = new QLabel(rightWidget);
    windowTitleLabel->setWordWrap(true);
    windowTitleLabel->setAlignment(Qt::AlignCenter);
    windowTitleLabel->setStyleSheet(R"(
        QLabel {
            background-color: rgba(255, 255, 255, 160);
            border: 1px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 1px;
            color: #003D7A;
            font-size: 12px;
        }
    )");
    QFont titleFont = windowTitleLabel->font();
    titleFont.setPointSize(9);
    windowTitleLabel->setFont(titleFont);

    handleLayout->addLayout(handleButtonLayout);
    handleLayout->addWidget(windowTitleLabel);
    rightLayout->addLayout(handleLayout);

    // 添加调试按钮
    captureBtn = new QPushButton("开始调试", rightWidget);
    captureBtn->setFixedSize(100, 30);
    captureBtn->setEnabled(false); // 初始状态禁用
    captureBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
        QPushButton:disabled {
            background-color: rgba(102, 204, 255, 200);
            color: rgba(0, 61, 122, 100);
            border: 2px solid rgba(200, 200, 200, 150);
        }
    )");
    connect(captureBtn, &QPushButton::clicked, this, &StarryCard::onCaptureAndRecognize);
    rightLayout->addWidget(captureBtn);

    // 添加调试选择下拉框
    QLabel *debugLabel = new QLabel("调试功能：");
    debugLabel->setStyleSheet(R"(
        QLabel {
            color: #003D7A;
            font-weight: bold;
            font-size: 12px;
        }
    )");
    rightLayout->addWidget(debugLabel);
    
    debugCombo = new QComboBox();
    debugCombo->addItems({"滚动条测试", "配方识别", "卡片识别", "四叶草识别", "香料识别", "刷新测试", "制卡测试", "全部功能"});
    debugCombo->setCurrentText("滚动条测试"); // 设置默认选择
    debugCombo->setStyleSheet(R"(
        QComboBox {
            background-color: rgba(255, 255, 255, 220);
            border: 2px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 4px 8px;
            color: #003D7A;
            font-size: 11px;
        }
        QComboBox:hover {
            background-color: rgba(102, 204, 255, 100);
        }
        QComboBox::drop-down {
            border: none;
            background-color: rgba(102, 204, 255, 150);
            border-radius: 3px;
        }
        QComboBox::down-arrow {
            image: url(:/images/icons/downArrow.svg);
            width: 12px;
            height: 12px;
            margin-right: 1px;
        }
    )");
    rightLayout->addWidget(debugCombo);

    // 添加主题选择标签和当前背景名称
    QHBoxLayout* themeLayout = new QHBoxLayout();
    QLabel *themeLabel = new QLabel("配置主题：");
    themeLabel->setStyleSheet(R"(
        QLabel {
            color: #003D7A;
            font-weight: bold;
            font-size: 14px;
        }
    )");
    currentBgLabel = new QLabel();
    currentBgLabel->setStyleSheet(R"(
        QLabel {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            font-size: 10px;
            border-radius: 3px;
            padding: 1px 2px;
        }
    )");
    updateCurrentBgLabel();
    themeLayout->addWidget(themeLabel);
    themeLayout->addWidget(currentBgLabel);
    rightLayout->addLayout(themeLayout);

    // 添加下拉框
    themeCombo = new QComboBox();
    themeCombo->addItems({"默认", "自定义"});
    themeCombo->setStyleSheet(R"(
        QComboBox {
            background-color: rgba(255, 255, 255, 220);
            border: 2px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 4px 8px;
            color: #003D7A;
            font-size: 11px;
        }
        QComboBox:hover {
            background-color: rgba(102, 204, 255, 100);
        }
        QComboBox::drop-down {
            border: none;
            background-color: rgba(102, 204, 255, 150);
            border-radius: 3px;
        }
        QComboBox::down-arrow {
            image: url(:/images/icons/downArrow.svg);
            width: 12px;
            height: 12px;
            margin-right: 1px;
        }
    )");
    connect(themeCombo, &QComboBox::currentTextChanged, this, &StarryCard::changeBackground);
    connect(themeCombo, &QComboBox::currentTextChanged, this, &StarryCard::updateSelectBtnState);
    rightLayout->addWidget(themeCombo);

    // 添加配方选择下拉框
    QLabel *recipeLabel = new QLabel("配方类型：");
    recipeLabel->setStyleSheet(R"(
        QLabel {
            color: #003D7A;
            font-weight: bold;
            font-size: 12px;
        }
    )");
    rightLayout->addWidget(recipeLabel);
    
    recipeCombo = new QComboBox();
    recipeCombo->setStyleSheet(R"(
        QComboBox {
            background-color: rgba(255, 255, 255, 220);
            border: 2px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 4px 8px;
            color: #003D7A;
            font-size: 11px;
        }
        QComboBox:hover {
            background-color: rgba(102, 204, 255, 100);
        }
        QComboBox::drop-down {
            border: none;
            background-color: rgba(102, 204, 255, 150);
            border-radius: 3px;
        }
        QComboBox::down-arrow {
            image: url(:/images/icons/downArrow.svg);
            width: 12px;
            height: 12px;
            margin-right: 1px;
        }
    )");
    rightLayout->addWidget(recipeCombo);

    // 添加选择自定义背景图的按钮
    selectCustomBgBtn = new QPushButton("选择自定义背景图");
    selectCustomBgBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
        QPushButton:disabled {
            background-color: rgba(102, 204, 255, 200);
            color: rgba(0, 61, 122, 100);
            border: 2px solid rgba(102, 204, 255, 255);
        }
    )");
    connect(selectCustomBgBtn, &QPushButton::clicked, this, &StarryCard::selectCustomBackground);
    selectCustomBgBtn->setEnabled(false);
    selectCustomBgBtn->setFixedHeight(40);
    rightLayout->addWidget(selectCustomBgBtn);

    rightLayout->addStretch(1);

    // 添加强化按钮（保持在底部）
    enhancementBtn = new QPushButton("开始强化");
    enhancementBtn->setFixedHeight(40);
    enhancementBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
    )");
    connect(enhancementBtn, &QPushButton::clicked, this, &StarryCard::startEnhancement);
    rightLayout->addWidget(enhancementBtn);

    // 将三个区域添加到水平布局
    topLayout->addWidget(leftWidget, 1);
    topLayout->addWidget(centerStack, 8);
    topLayout->addWidget(rightWidget, 3);

    // 将水平布局添加到主垂直布局
    mainLayout->addLayout(topLayout, 1);
}

StarryCard::~StarryCard()
{
    // 清理线程
    if (enhancementThread && enhancementThread->isRunning()) {
        enhancementThread->quit();
        enhancementThread->wait();
    }
    
    // 清理全局像素数据
    if (globalPixelData) {
        delete[] globalPixelData;
        globalPixelData = nullptr;
        globalBitmapWidth = 0;
        globalBitmapHeight = 0;
    }

    // 清理RecipeRecognizer
    if (recipeRecognizer) {
        delete recipeRecognizer;
        recipeRecognizer = nullptr;
    }

    // 清理CardRecognizer
    if (cardRecognizer) {
        delete cardRecognizer;
        cardRecognizer = nullptr;
    }

    // 清理定时器
    if (configSaveTimer) {
        configSaveTimer->stop();
        delete configSaveTimer;
        configSaveTimer = nullptr;
    }
    if (spiceSaveTimer) {
        spiceSaveTimer->stop();
        delete spiceSaveTimer;
        spiceSaveTimer = nullptr;
    }
}

void StarryCard::startMouseTracking() {
    // 确保DPI感知已启用
    SetProcessDPIAware();
    
    isTracking = true;
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    addLog("开始获取窗口句柄...", LogType::Info);
}

// 找到游戏窗口或选服窗口hWnd所属的大厅窗口
HWND StarryCard::GetHallWindow(HWND hWnd)
{
    HWND hWndParent = GetParent(hWnd); // 获得父窗口
    if (hWndParent == NULL)
        return NULL; // 没有父窗口，返回NULL
    wchar_t className[256];
    GetClassNameW(hWndParent, className, 256);                    // 获取窗口类名
    if (wcscmp(className, L"ApolloRuntimeContentWindow") == 0) // 如果是微端窗口，直接返回
        return hWndParent;
    if (wcscmp(className, L"WrapperNativeWindowClass") == 0) // 如果是大厅游戏窗口的上级窗口，向上找四层
    {
        hWndParent = GetParent(hWndParent); // 找到Chrome_WidgetWin_0层
        hWndParent = GetParent(hWndParent); // 找到CefBrowserWindow层
        hWndParent = GetParent(hWndParent); // 找到TabContentWnd层
        return GetParent(hWndParent);       // 找到DUIWindow层（大厅）并返回
    }
    if (wcscmp(className, L"Chrome_WidgetWin_0") == 0) // 如果是大厅选服窗口的上级窗口，向上找三层
    {
        hWndParent = GetParent(hWndParent); // 找到CefBrowserWindow层
        hWndParent = GetParent(hWndParent); // 找到TabContentWnd层
        return GetParent(hWndParent);       // 找到DUIWindow层（大厅）并返回
    }
    return NULL;
}

// 判断游戏窗口hWnd是否能识别图像
bool StarryCard::IsGameWindowVisible(HWND hWnd)
{
  return IsWindowVisible(hWnd) && !IsIconic(GetHallWindow(hWnd));//窗口可见且大厅没有最小化
}

//点击刷新按钮并记录
void StarryCard::clickRefresh()
{
    int RefreshX = 228, RefreshY = 44;
    leftClickDPI(hwndHall, RefreshX, RefreshY); // 重新点击刷新按钮
    addLog(QString("点击刷新按钮:(%1,%2)").arg(RefreshX).arg(RefreshY), LogType::Info);
}

// 找到游戏窗口
HWND StarryCard::getGameWindow(HWND hWndChild) {
    // 查找二级子窗口
    HWND hWnd2 = FindWindowEx(hWndChild, nullptr, L"CefBrowserWindow", nullptr);
    if (!hWnd2) {
        qDebug() << "Failed to find CefBrowserWindow child window";
        return nullptr;
    }

    // 查找三级子窗口
    HWND hWnd3 = FindWindowEx(hWnd2, nullptr, L"Chrome_WidgetWin_0", nullptr);
    if (!hWnd3) {
        qDebug() << "Failed to find Chrome_WidgetWin_0 child window";
        return nullptr;
    }

    // 查找四级子窗口
    HWND hWnd4 = FindWindowEx(hWnd3, nullptr, L"WrapperNativeWindowClass", nullptr);
    if (!hWnd4) {
        qDebug() << "Failed to find WrapperNativeWindowClass child window at time " << QTime::currentTime().toString("hh:mm:ss");
        return nullptr;
    }

    // 查找五级子窗口
    return FindWindowEx(hWnd4, nullptr, L"NativeWindowClass", nullptr);
}

// 找到游戏大厅当前显示的游戏窗口，找不到返回NULL
HWND StarryCard::getActiveGameWindow(HWND hwndHall) {
    // 按微端查找小号窗口
    HWND hWndChild = FindWindowEx(hwndHall, nullptr, L"WebPluginView", nullptr);
    if (hWndChild) {
        return hWndChild; // 找到了直接返回游戏窗口
    }

    hWndChild = nullptr;
    while (true) {
        // 查找一个小号窗口
        hWndChild = FindWindowEx(hwndHall, hWndChild, L"TabContentWnd", nullptr);
        if (!hWndChild) {
            qDebug() << "No more TabContentWnd child windows found at " << QTime::currentTime().toString("hh:mm:ss");
            return nullptr; // 没有小号窗口了，还没找到活跃窗口，就返回NULL
        }

        HWND hWndGame = getGameWindow(hWndChild);
        if (hWndGame) {
            return hWndGame;
        }
    }
}

void StarryCard::stopMouseTracking() {
    isTracking = false;
    setCursor(Qt::ArrowCursor);
    setMouseTracking(false);

    QPoint globalPos = QCursor::pos();
    HWND hwnd = WindowUtils::getWindowFromPoint(globalPos);
    
    if (!hwnd) {
        hwndGame = nullptr;
        updateHandleDisplay(nullptr);
        
        // 禁用调试按钮
        if (captureBtn) {
            captureBtn->setEnabled(false);
        }
        
        QMessageBox::warning(
            this,
            "错误",
            "未能获取窗口句柄，请重试。"
        );
        return;
    }

    if (!WindowUtils::isGameWindow(hwnd)) {
        hwndGame = nullptr;
        updateHandleDisplay(hwnd);
        
        // 禁用调试按钮
        if (captureBtn) {
            captureBtn->setEnabled(false);
        }
        
        QMessageBox::warning(
            this,
            "未找到游戏窗口",
            "未找到游戏窗口。本程序只能用于\n360游戏大厅（极速模式）或微端。"
        );
        return;
    }

    // 成功获取目标窗口
    hwndGame = hwnd;              // 游戏窗口
    hwndHall = GetHallWindow(hwnd); // 大厅窗口
    updateHandleDisplay(hwnd);
    
    // 启用调试按钮
    if (captureBtn) {
        captureBtn->setEnabled(true);
    }
    
    qDebug() << "Successfully captured game window handle:" << hwnd;
}

void StarryCard::startEnhancement()
{
    if (!isEnhancing && enhancementBtn) {
        // 在主线程中读取UI控件的值并保存到成员变量
        if (maxEnhancementLevelSpinBox && minEnhancementLevelSpinBox) {
        maxEnhancementLevel = maxEnhancementLevelSpinBox->value();
        minEnhancementLevel = minEnhancementLevelSpinBox->value();
        }
        
        // 在主线程中预先加载所需的卡片类型
        requiredCardTypes = getRequiredCardTypesFromConfig();
        
        // 在主线程中预先加载全局强化配置
        if (loadGlobalEnhancementConfig() && loadGlobalSpiceConfig()) {
            addLog("全局强化配置加载成功", LogType::Success);
        } else {
            addLog("全局强化配置加载失败，使用默认配置", LogType::Warning);
        }
        
        enhancementBtn->setText("停止强化");
        emit startEnhancementSignal();
    } else {
        stopEnhancement();
    }
}

void StarryCard::showWarningMessage(const QString& title, const QString& message)
{
    QMessageBox::warning(this, title, message);
}

void StarryCard::onEnhancementFinished()
{
    if (enhancementBtn) {
        isEnhancing = false;
        enhancementBtn->setText("开始强化");
        
        // 清空缓存的卡片类型
        requiredCardTypes.clear();
        
        addLog("强化流程结束", LogType::Info);
    }
}

void StarryCard::stopEnhancement()
{
    if (enhancementBtn) {
        isEnhancing = false;
        enhancementBtn->setText("开始强化");
        
        // 清空缓存的卡片类型
        requiredCardTypes.clear();
        
        addLog("停止强化流程", LogType::Warning);
    }
}

void StarryCard::trackMousePosition(QPoint pos) {
    if (isTracking) {
        // qDebug() << "Mouse position:" << pos;
    }
}

void StarryCard::mouseMoveEvent(QMouseEvent *event) {
    if (isTracking) {
        trackMousePosition(event->globalPosition().toPoint());
    }
    QMainWindow::mouseMoveEvent(event);
}

void StarryCard::mouseReleaseEvent(QMouseEvent *event) {
    if (isTracking && event->button() == Qt::LeftButton) {
        stopMouseTracking();
    }
    QMainWindow::mouseReleaseEvent(event);
}

void StarryCard::handleButtonRelease() {
    if (isTracking) {
        stopMouseTracking();
    }
}

void StarryCard::setBackground(const QString &imagePath)
{
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        QPalette palette = this->palette();
        int windowWidth = this->width();
        int windowHeight = this->height();

        // 按比例缩放图片，使图片至少有一个维度与窗口尺寸一致
        QPixmap scaledPixmap = pixmap.scaled(
            windowWidth, windowHeight,
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
            );

        // 计算裁剪区域，保证从图片中心裁剪
        int scaledWidth = scaledPixmap.width();
        int scaledHeight = scaledPixmap.height();
        int x = (scaledWidth - windowWidth) / 2;
        int y = (scaledHeight - windowHeight) / 2;

        // 裁剪图片
        QPixmap croppedPixmap = scaledPixmap.copy(x, y, windowWidth, windowHeight);

        palette.setBrush(QPalette::Window, QBrush(croppedPixmap));
        this->setPalette(palette);
    } else {
        qDebug() << "图片加载失败，请检查路径: " << imagePath;
        qDebug() << "当前目录: " << QDir::currentPath();
    }
}

void StarryCard::changeBackground(const QString &theme)
{
    if (theme == "默认") {
        setBackground(defaultBgPath);
    } else if (theme == "自定义") {
        if (!customBgPath.isEmpty() && QFile::exists(customBgPath)) {
            setBackground(customBgPath);
        } else {
            selectCustomBackground();
        }
    }
    updateCurrentBgLabel();
}

void StarryCard::selectCustomBackground()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "选择背景图片", "", "图片文件 (*.png *.jpg *.jpeg)"
    );
    if (!filePath.isEmpty()) {
        customBgPath = filePath;
        saveCustomBgPath();
        setBackground(filePath);
        updateCurrentBgLabel();
    }
}

void StarryCard::updateSelectBtnState(const QString &theme)
{
    selectCustomBgBtn->setEnabled(theme == "自定义");
}

void StarryCard::saveCustomBgPath()
{
    // 保存自定义背景图片路径到 JSON 文件
    QJsonObject config;
    config["custom_bg_path"] = customBgPath;
    QJsonDocument doc(config);
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void StarryCard::loadCustomBgPath()
{
    // 从 JSON 文件读取自定义背景图片路径
    QFile file(jsonPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject config = doc.object();
            customBgPath = config["custom_bg_path"].toString();
        }
        file.close();
    }
}

void StarryCard::updateHandleDisplay(HWND hwnd) {
    if (!handleDisplayEdit || !windowTitleLabel) {
        return;
    }

    // 找到调试按钮并更新其状态
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (const auto& btn : std::as_const(buttons)) {
        if (btn->text() == "开始调试") {
            btn->setEnabled(hwnd != nullptr);
            break;
        }
    }

    if (hwnd) {
        QString title = WindowUtils::getWindowTitle(hwndHall); // 获取大厅窗口标题
        handleDisplayEdit->setText(QString::number(reinterpret_cast<quintptr>(hwnd), 10)); // 显示游戏窗口句柄
        windowTitleLabel->setText(title);
        addLog(QString("已绑定窗口：%1，句柄：%2").arg(title, QString::number(reinterpret_cast<quintptr>(hwnd), 10)), LogType::Success);
    } else {
        handleDisplayEdit->setText("未获取到句柄");
        windowTitleLabel->setText("");
        addLog("窗口绑定失败", LogType::Error);
    }
}

void StarryCard::addLog(const QString& message, LogType type)
{
    // 确保在主线程中执行UI操作
    if (QThread::currentThread() != this->thread()) {
        qWarning() << "addLog called from wrong thread!";
        return;
    }
    
    if (!logTextEdit) {
        return;
    }
    
    // 获取当前时间
    QString timestamp = QTime::currentTime().toString("hh:mm:ss");
    
    // 根据日志类型设置颜色
    QString colorStyle;
    switch (type) {
        case LogType::Success:
            colorStyle = "color: green;";
            break;
        case LogType::Warning:
            colorStyle = "color: orange;";
            break;
        case LogType::Error:
            colorStyle = "color: red;";
            break;
        case LogType::Info:
            colorStyle = "color: blue;";
            break;
        default:
            colorStyle = "color: black;";
    }
    
    // 格式化日志消息
    QString formattedMessage = QString("<div style='%1'>[%2] %3</div>")
                                .arg(colorStyle, timestamp, message);
    
    // 添加新日志
    logTextEdit->append(formattedMessage);

    // 使用更简单的行数限制策略，避免频繁的文档操作
    static int logCount = 0;
    logCount++;
    
    // 每50条检查一次，减少频繁操作
    if (logCount % 50 == 0) {
        QTextDocument* doc = logTextEdit->document();
        if (doc && doc->blockCount() > 300) {
            // 使用setMaximumBlockCount来自动限制行数
            logTextEdit->document()->setMaximumBlockCount(256);
        }
    }
    
    // 滚动到最新日志 - 减少滚动操作的频率
    if (logCount % 5 == 0) {
        QScrollBar* scrollBar = logTextEdit->verticalScrollBar();
        if (scrollBar) {
            scrollBar->setValue(scrollBar->maximum());
        }
    }
}

void StarryCard::updateCurrentBgLabel()
{
    if (!currentBgLabel || !themeCombo) {
        return;
    }

    QString bgName;
    if (themeCombo->currentText() == "默认") {
        QFileInfo fileInfo(defaultBgPath);
        bgName = fileInfo.baseName();
    } else {
        QFileInfo fileInfo(customBgPath);
        bgName = fileInfo.baseName();
    }
    
    // 限制显示长度为8个字符
    if (bgName.length() > 8) {
        bgName = bgName.left(6) + "...";
    }
    
    currentBgLabel->setText(bgName);
}

// 获取DPI缩放因子
int StarryCard::getDPIFromDC()
{
    HDC hdc = GetDC(GetDesktopWindow());
    
    int dpi = 96;  // 默认DPI
    if (hdc) {
        dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(GetDesktopWindow(), hdc);
    }
    return dpi;
}

QImage StarryCard::captureWindowByHandle(HWND hwnd, const QString& windowName)
{
    if (!hwnd || !IsWindow(hwnd)) {
        addLog(QString("无效的窗口句柄: %1").arg(windowName), LogType::Error);
        return QImage();
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

    // qDebug() << QString("成功截取%1窗口图像：%2x%3").arg(windowName).arg(width).arg(height);
    return image;
}

QImage StarryCard::captureImageRegion(const QImage& sourceImage, const QRect& rect, const QString& filename)
{
    // 检查源图像是否有效
    if (sourceImage.isNull()) {
        qDebug() << "源图像无效，无法截取区域";
        return QImage();
    }
    
    // 验证QRect参数的有效性
    if (!rect.isValid() || rect.isEmpty()) {
        qDebug() << QString("无效的截取区域：(%1,%2,%3,%4)")
                    .arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
        return QImage();
    }
    
    // 检查截取区域是否超出源图像边界
    QRect imageRect(0, 0, sourceImage.width(), sourceImage.height());
    if (!imageRect.contains(rect)) {
        qDebug() << QString("截取区域超出图像边界。图像尺寸：%1x%2，截取区域：(%3,%4,%5,%6)")
                    .arg(sourceImage.width()).arg(sourceImage.height())
                    .arg(rect.x()).arg(rect.y())
                    .arg(rect.width()).arg(rect.height());
        return QImage();
    }
    
    // 截取指定区域
    QImage regionImage = sourceImage.copy(rect);
    if (regionImage.isNull()) {
        qDebug() << "截取图像区域失败";
        return QImage();
    }
    
    // 确保screenshots文件夹存在
    QString appDir = QCoreApplication::applicationDirPath();
    QString screenshotsDir = appDir + "/screenshots";
    QDir dir(screenshotsDir);
    if (!dir.exists()) {
        if (!dir.mkpath(screenshotsDir)) {
            qDebug() << "创建screenshots文件夹失败";
            return QImage();
        }
    }
    
    // 生成文件名
    QString saveFilename;
    if (filename.isEmpty()) {
        saveFilename = QString("%1/region_%2_%3_%4x%5.png")
                      .arg(screenshotsDir)
                      .arg(rect.x()).arg(rect.y())
                      .arg(rect.width()).arg(rect.height());
    } else {
        // 如果提供了文件名但没有扩展名，则添加.png
        QString baseFilename = filename;
        if (!baseFilename.endsWith(".png", Qt::CaseInsensitive)) {
            baseFilename += ".png";
        }
        saveFilename = QString("%1/%2").arg(screenshotsDir, baseFilename);
    }
    
    // 保存截取的图像
    if (regionImage.save(saveFilename)) {
        qDebug() << QString("区域图像保存成功：%1 (尺寸：%2x%3)")
                    .arg(saveFilename,
                         QString::number(regionImage.width()),
                         QString::number(regionImage.height()));
    } else {
        qDebug() << QString("区域图像保存失败：%1").arg(saveFilename);
        // 即使保存失败，仍然返回截取到的图像
    }
    
    return regionImage;
}

void StarryCard::showRecognitionResults(const QVector<CardInfo>& results)
{
    QString message = "识别到的卡片：\n";
    for (const auto& result : results) {
        message += (result.name) + 
                  QString(" (%1星, %2)\n").arg(result.level)
                  .arg(result.isBound ? "已绑定" : "未绑定");
    }
    
    // QMessageBox::information(this, "识别结果", message);
}

void StarryCard::onCaptureAndRecognize()
{
    if (!IsGameWindowVisible(hwndGame)) {
        QMessageBox::warning(this, "错误", "游戏窗口不可见，请先打开游戏窗口！");
        return;
    }

    QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
    if (screenshot.isNull()) {
        QMessageBox::warning(this, "错误", "截图失败");
        return;
    }

    // 获取当前应用程序的目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString screenshotsDir = appDir + "/screenshots";
    QDir dir(screenshotsDir);

    // 尝试创建screenshots文件夹
    if (!dir.exists()) {
        addLog("正在创建screenshots文件夹...", LogType::Info);
        if (!dir.mkpath(screenshotsDir)) {
            addLog("创建screenshots文件夹失败", LogType::Error);
            QMessageBox::warning(this, "错误", "无法创建screenshots文件夹");
            return;
        }
    }

    // 生成固定的文件名
    QString filename = QString("%1/screenshot_main.png").arg(screenshotsDir);

    // 保存截图
    if (screenshot.save(filename)) {
        addLog("截图保存成功", LogType::Success);
    } else {
        addLog("截图保存失败", LogType::Error);
        QMessageBox::warning(this, "错误", "无法保存截图");
        return;
    }

    // 根据调试选择执行相应的功能
    if (!debugCombo) {
        addLog("调试选择下拉框未初始化", LogType::Error);
        return;
    }
    
    QString debugMode = debugCombo->currentText();
    addLog(QString("执行调试功能: %1").arg(debugMode), LogType::Info);
    
    // if (debugMode == "位置跳转" || debugMode == "全部功能") {
    //     // 执行位置跳转功能
    //     addLog("开始位置跳转...", LogType::Info);
    //     goToPage(PageType::CardEnhance); // 卡片强化
    //     goToPage(PageType::CardProduce); // 卡片制作
    // }

    if (debugMode == "滚动条测试" || debugMode == "全部功能") {
        addLog("开始滚动条测试...", LogType::Info);
        if(resetScrollBar())
        {
            addLog("滚动条重置成功", LogType::Success);
        }
        else
        {
            addLog("滚动条重置失败", LogType::Error);
        }

        QImage screenshotScrollBar = captureWindowByHandle(hwndGame,"主页面");

        int length = getLengthOfScrollBar(screenshotScrollBar);
        int scrollOnceLength = (length * 7 /8) + 2;
        if(length > 0)
        {
            addLog(QString("滚动条长度: %1").arg(length), LogType::Success);
        }
        else
        {
            addLog("无法获取滚动条长度", LogType::Error);
        }

        QRect cardAreaRoi = QRect(559, 91, 343, 456);
        QString appDir = QCoreApplication::applicationDirPath();
        QString screenshotsDir = appDir + "/screenshots";
        int position = getPositionOfScrollBar(screenshot);
        addLog(QString("初始滚动条位置: %1").arg(position), LogType::Success);
        for(int i = 0; i < 10; i++)
        {
            QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
            QImage cardArea = screenshot.copy(cardAreaRoi);
            cardArea.save(QString("%1/cardArea_%2.png").arg(screenshotsDir).arg(i));
            if(checkSynHousePosState(screenshot, ENHANCE_SCROLL_BAR_BOTTOM, "enhanceScrollBottom"))
            {
                addLog(QString("滚动条底部识别成功: %1").arg(i), LogType::Success);
                break;
            }
            fastMouseDrag(ENHANCE_SCROLL_TOP.x(), ENHANCE_SCROLL_TOP.y() + i * length, scrollOnceLength, true);
            int j = 0;
            while(j < 100)
            {
                sleepByQElapsedTimer(50);
                QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
                if(getPositionOfScrollBar(screenshot) != position)
                {
                    position = getPositionOfScrollBar(screenshot);
                    break;
                }
                j++;
            }
            addLog(QString("滚动条位置: %1").arg(position), LogType::Success);
        }
        
    }
    
    if (debugMode == "配方识别" || debugMode == "全部功能") {
        // 执行网格线调试
        qDebug() << "开始网格线调试...";
        recipeRecognizer->debugGridLines(screenshot);
        // 执行配方识别功能
        qDebug() << "开始配方识别...";
        
        // 选择要匹配的配方模板（动态获取可用的配方类型）
        QStringList availableRecipes = getAvailableRecipeTypes();
        if (availableRecipes.isEmpty()) {
            addLog("没有可用的配方模板，无法进行识别", LogType::Error);
        } else {
            // 从UI中选择配方类型，必须明确选择
            QString targetRecipe;
            if (recipeCombo && recipeCombo->isEnabled() && recipeCombo->currentText() != "无可用配方") {
                targetRecipe = recipeCombo->currentText();
                addLog(QString("从UI选择配方类型: %1").arg(targetRecipe));
            } else {
                addLog("UI未选择配方类型，配方识别失败", LogType::Error);
                addLog(QString("可用配方类型: %1").arg(availableRecipes.join(", ")));
                addLog("请在下拉框中选择要识别的配方类型");
                return; // 直接返回错误，不执行识别
            }
            
            if (!availableRecipes.contains(targetRecipe)) {
                addLog(QString("选择的配方类型 '%1' 不存在，配方识别失败").arg(targetRecipe), LogType::Error);
                addLog(QString("可用配方类型: %1").arg(availableRecipes.join(", ")));
                return; // 直接返回错误，不执行识别
            }
            addLog(QString("可用配方类型: %1").arg(availableRecipes.join(", ")));
            addLog(QString("选择匹配模板: %1").arg(targetRecipe));
            
            // 执行带翻页功能的配方识别
            if (!hwndGame || !IsWindow(hwndGame)) {
                addLog("游戏窗口句柄无效，无法进行配方识别", LogType::Error);
                return;
            }
            
            // 设置RecipeRecognizer的参数
            recipeRecognizer->setDPI(DPI);
            recipeRecognizer->setGameWindow(hwndGame);
            
            addLog(QString("开始配方识别，游戏窗口句柄: %1, DPI: %2").arg(reinterpret_cast<quintptr>(hwndGame)).arg(DPI));
            
            recipeRecognizer->recognizeRecipeWithPaging(screenshot, targetRecipe, hwndGame);
        }
    }
    
    if (debugMode == "卡片识别" || debugMode == "全部功能") {
        // 执行卡片识别功能
        addLog("开始识别卡片...", LogType::Info);
        
        // 获取配置文件中需要的卡片类型
        QStringList requiredCardTypes = getRequiredCardTypesFromConfig();
        
        QVector<CardInfo> results;
        if (!requiredCardTypes.isEmpty()) {
            // 使用针对性识别，只识别配置中需要的卡片类型
            results = cardRecognizer->recognizeCards(screenshot, requiredCardTypes);
            addLog(QString("识别目标卡片类型: %1").arg(requiredCardTypes.join(", ")), LogType::Info);
        }

        if (results.empty()) {
            addLog("未识别到任何卡片", LogType::Warning);
        } else {
            addLog(QString("识别到 %1 张卡片").arg(results.size()), LogType::Success);
        }

        // 显示识别结果
        showRecognitionResults(results);
    }
    
    if (debugMode == "四叶草识别" || debugMode == "全部功能") {
        // 执行四叶草识别功能
        addLog("开始测试四叶草识别功能...", LogType::Info);
        qDebug() << "=== 开始四叶草识别测试 ===";
        
        // 测试识别1级四叶草（任意绑定状态）
        QPair<bool, bool> result = recognizeClover("1级", false, true);
        if (result.first) {
            addLog(QString("四叶草识别测试成功！绑定状态: %1").arg(result.second ? "绑定" : "未绑定"), LogType::Success);
        } else {
            addLog("四叶草识别测试失败", LogType::Warning);
        }
        qDebug() << "=== 四叶草识别测试结束 ===";
    }
    
    if (debugMode == "香料识别" || debugMode == "全部功能") {
        // 执行香料识别功能
        addLog("开始测试香料识别功能...", LogType::Info);
        qDebug() << "=== 开始香料识别测试 ===";
        
        // 测试1: 找到不绑定的天然香料
        addLog("测试1: 找到不绑定的天然香料", LogType::Info);
        QPair<bool, bool> spiceResult1 = recognizeSpice("天然香料", false, true);
        if (spiceResult1.first) {
            addLog("成功识别到不绑定的天然香料", LogType::Success);
        } else {
            addLog("未找到不绑定的天然香料", LogType::Warning);
        }
        
        sleepByQElapsedTimer(500);
        QImage screenshotSpice = captureWindowByHandle(hwndGame,"合成屋");

        if(checkSpicePosState(screenshotSpice, SPICE_AREA_HOUSE, "天然香料")) {
            addLog("合成屋香料区域天然香料识别成功", LogType::Success);
        } else {
            addLog("合成屋香料区域天然香料识别失败", LogType::Error);
        }

        // 测试2: 找到绑定的皇室香料
        addLog("测试2: 找到绑定的皇室香料", LogType::Info);
        QPair<bool, bool> spiceResult2 = recognizeSpice("皇室香料", true, false);
        if (spiceResult2.first) {
            addLog("成功识别到绑定的皇室香料", LogType::Success);
        } else {
            addLog("未找到绑定的皇室香料", LogType::Warning);
        }

        sleepByQElapsedTimer(500);
        screenshotSpice = captureWindowByHandle(hwndGame,"合成屋");

        if(checkSpicePosState(screenshotSpice, SPICE_AREA_HOUSE, "皇室香料")) {
            addLog("合成屋香料区域皇室香料识别成功", LogType::Success);
        } else {
            addLog("合成屋香料区域皇室香料识别失败", LogType::Error);
        }
    }

    if (debugMode == "刷新测试" || debugMode == "全部功能") {
        // 执行刷新测试功能
        addLog("开始刷新测试...", LogType::Info);
        refreshGameWindow();

        sleepByQElapsedTimer(4000); // 等待4秒

        closeHealthTip();
    }
    
    if (debugMode == "制卡测试") {
        // 执行制卡测试功能
        addLog("开始制卡测试...", LogType::Info);
        performCardMaking();
    }
    
    addLog(QString("调试功能 '%1' 执行完成").arg(debugMode), LogType::Success);
}

QWidget* StarryCard::createEnhancementConfigPage()
{
    QWidget* page = new QWidget();
    page->setStyleSheet("background-color: transparent;");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0); // 减少边距以增加表格可用空间
    layout->setSpacing(0);

    // 创建标题
    QLabel* titleLabel = new QLabel("强化方案配置");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(R"(
        font-size: 16px; 
        font-weight: bold; 
        color: #003D7A;
        background-color: rgba(125, 197, 255, 150);
        border-radius: 8px;
        padding: 8px;
        margin: 5px 0;
    )");
    layout->addWidget(titleLabel);
    
    // 创建表格
    enhancementTable = new QTableWidget(14, 7);
    enhancementTable->setStyleSheet(R"(
        QTableWidget {
            gridline-color: rgba(102, 204, 255, 120);
            background-color: rgba(255, 255, 255, 160);
            alternate-background-color: rgba(102, 204, 255, 80);
            border: 2px solid rgba(102, 204, 255, 200);
            border-radius: 8px;
        }
        QTableWidget::item {
            padding: 3px;
            border: none;
        }
        QHeaderView::section {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            padding: 6px;
            border: 1px solid rgba(102, 204, 255, 150);
            font-weight: bold;
            font-size: 11px;
        }
        QComboBox {
            background-color: rgba(255, 255, 255, 220);
            border: 1px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 2px 4px;
            color: #003D7A;
        }
        QComboBox:hover {
            background-color: rgba(102, 204, 255, 100);
        }
        QComboBox::drop-down {
            border: none;
            background-color: rgba(102, 204, 255, 150);
            border-radius: 3px;
        }
        QComboBox::down-arrow {
            image: url(:/images/icons/downArrow.svg);
            width: 12px;
            height: 12px;
            margin-right: 1px;
        }
    )");
    
    // 设置表头
    QStringList headers = {"强化等级", "副卡1", "副卡2", "副卡3", "四叶草", "四叶草状态", "卡片设置"};
    enhancementTable->setHorizontalHeaderLabels(headers);
    
    // 设置表格属性
    enhancementTable->verticalHeader()->setVisible(false);
    enhancementTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    enhancementTable->setAlternatingRowColors(true);
    enhancementTable->horizontalHeader()->setStretchLastSection(true);
    enhancementTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    enhancementTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 设置表格的大小策略为固定宽度和限制高度
    enhancementTable->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    
    // 设置固定宽度表格，不压缩其他区域
    enhancementTable->setFixedWidth(540);  // 固定宽度，后续可手动调整
    
    // 设置具体的列宽而不是自动拉伸
    enhancementTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    enhancementTable->setColumnWidth(0, 65);   // 强化等级
    enhancementTable->setColumnWidth(1, 65);   // 副卡1
    enhancementTable->setColumnWidth(2, 65);   // 副卡2
    enhancementTable->setColumnWidth(3, 60);   // 副卡3
    enhancementTable->setColumnWidth(4, 70);   // 四叶草
    enhancementTable->setColumnWidth(5, 110);   // 四叶草状态
    enhancementTable->setColumnWidth(6, 70);   // 卡片设置
    
    // 填充表格内容
    for (int row = 0; row < 14; ++row) {
        // 强化等级标签
        QLabel* levelLabel = new QLabel(QString("%1→%2").arg(row).arg(row + 1));
        levelLabel->setAlignment(Qt::AlignCenter);
        levelLabel->setStyleSheet(R"(
            font-weight: bold; 
            color: #003D7A;
            background-color: rgba(102, 204, 255, 100);
            border-radius: 4px;
            padding: 2px;
        )");
        enhancementTable->setCellWidget(row, 0, levelLabel);
        
        // 副卡1下拉框（必选）
        QComboBox* subCard1 = new QComboBox();
        for (int star = getMinSubCardLevel(row); star <= getMaxSubCardLevel(row); ++star) {
            subCard1->addItem(QString("%1星").arg(star));
        }
        subCard1->setProperty("row", row);
        subCard1->setProperty("type", "subcard1");
        connect(subCard1, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                this, &StarryCard::onEnhancementConfigChanged);
        enhancementTable->setCellWidget(row, 1, subCard1);
        
        // 副卡2下拉框（可选）
        QComboBox* subCard2 = new QComboBox();
        subCard2->addItem("无");
        for (int star = getMinSubCardLevel(row); star <= getMaxSubCardLevel(row); ++star) {
            subCard2->addItem(QString("%1星").arg(star));
        }
        subCard2->setProperty("row", row);
        subCard2->setProperty("type", "subcard2");
        connect(subCard2, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                this, &StarryCard::onEnhancementConfigChanged);
        enhancementTable->setCellWidget(row, 2, subCard2);
        
        // 副卡3下拉框（可选）
        QComboBox* subCard3 = new QComboBox();
        subCard3->addItem("无");
        for (int star = getMinSubCardLevel(row); star <= getMaxSubCardLevel(row); ++star) {
            subCard3->addItem(QString("%1星").arg(star));
        }
        subCard3->setProperty("row", row);
        subCard3->setProperty("type", "subcard3");
        connect(subCard3, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                this, &StarryCard::onEnhancementConfigChanged);
        enhancementTable->setCellWidget(row, 3, subCard3);
        
        // 四叶草下拉框
        QComboBox* clover = new QComboBox();
        QStringList cloverTypes = {"无", "1级", "2级", "3级", "4级", "5级", "6级", "S", "SS", "SSS", "SSR", "蛇草"};
        clover->addItems(cloverTypes);
        clover->setProperty("row", row);
        clover->setProperty("type", "clover");
        connect(clover, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                this, &StarryCard::onEnhancementConfigChanged);
        enhancementTable->setCellWidget(row, 4, clover);
        
        // 四叶草状态下拉框
        QWidget* cloverStateWidget = new QWidget();
        QHBoxLayout* cloverStateLayout = new QHBoxLayout(cloverStateWidget);
        cloverStateLayout->setContentsMargins(2, 2, 2, 2);
        cloverStateLayout->setSpacing(2);
        
        QCheckBox* bindCheck = new QCheckBox("绑定");
        QCheckBox* unboundCheck = new QCheckBox("不绑");
        bindCheck->setProperty("row", row);
        bindCheck->setProperty("type", "cloverbind");
        unboundCheck->setProperty("row", row);
        unboundCheck->setProperty("type", "cloverunbound");
        
        connect(bindCheck, &QCheckBox::checkStateChanged, this, &StarryCard::onEnhancementConfigChanged);
        connect(unboundCheck, &QCheckBox::checkStateChanged, this, &StarryCard::onEnhancementConfigChanged);
        
        cloverStateLayout->addWidget(bindCheck);
        cloverStateLayout->addWidget(unboundCheck);
        cloverStateLayout->setAlignment(Qt::AlignCenter);
        
        enhancementTable->setCellWidget(row, 5, cloverStateWidget);
        
        // 卡片设置按钮
        QPushButton* cardSettingBtn = new QPushButton("设置");
        cardSettingBtn->setProperty("row", row);
        cardSettingBtn->setStyleSheet(R"(
            QPushButton {
                background-color: rgba(102, 204, 255, 200);
                color: #003D7A;
                border: 2px solid rgba(102, 204, 255, 255);
                border-radius: 4px;
                font-weight: bold;
                font-size: 10px;
                padding: 2px;
            }
            QPushButton:hover {
                background-color: rgba(102, 204, 255, 255);
                color: white;
            }
            QPushButton:pressed {
                background-color: rgba(0, 61, 122, 200);
                color: white;
            }
        )");
        connect(cardSettingBtn, &QPushButton::clicked, this, &StarryCard::onCardSettingButtonClicked);
        enhancementTable->setCellWidget(row, 6, cardSettingBtn);
    }
    
    layout->addWidget(enhancementTable);
    
    // 添加强化等级设置（最低和最高在同一行）
    QHBoxLayout* levelSettingsLayout = new QHBoxLayout();
    levelSettingsLayout->setSpacing(20);
    levelSettingsLayout->setContentsMargins(10, 5, 10, 5);
    
    // 最低强化等级设置
    QLabel* minLevelLabel = new QLabel("最低强化星级:");
    minLevelLabel->setStyleSheet(R"(
        QLabel {
            color: #003D7A;
            font-weight: bold;
            font-size: 14px;
            background-color: rgba(125, 197, 255, 100);
            border-radius: 4px;
            padding: 5px 8px;
        }
    )");
    
    minEnhancementLevelSpinBox = new QSpinBox();
    minEnhancementLevelSpinBox->setMinimum(1);
    minEnhancementLevelSpinBox->setMaximum(14);
    minEnhancementLevelSpinBox->setValue(1); // 默认为1级
    minEnhancementLevelSpinBox->setSuffix(" 星");
    minEnhancementLevelSpinBox->setStyleSheet(R"(
        QSpinBox {
            background-color: rgba(255, 255, 255, 220);
            border: 2px solid rgba(102, 204, 255, 150);
            border-radius: 6px;
            padding: 4px 8px;
            color: #003D7A;
            font-weight: bold;
            font-size: 13px;
            min-width: 80px;
        }
        QSpinBox:hover {
            background-color: rgba(102, 204, 255, 100);
            border: 2px solid rgba(102, 204, 255, 200);
        }
        QSpinBox:focus {
            background-color: rgba(255, 255, 255, 255);
            border: 2px solid rgba(33, 150, 243, 200);
        }
        QSpinBox::up-button {
            image: url(:/images/icons/uparrow_1f.svg);
            background-color: rgba(102, 204, 255, 150);
            border: 1px solid rgba(102, 204, 255, 200);
            border-radius: 3px;
            width: 16px;
            margin-right: 2px;
        }
        QSpinBox::up-button:hover {
            background-color: rgba(102, 204, 255, 200);
        }
        QSpinBox::down-button {
            image: url(:/images/icons/downarrow_1f.svg);
            background-color: rgba(102, 204, 255, 150);
            border: 1px solid rgba(102, 204, 255, 200);
            border-radius: 3px;
            width: 16px;
            margin-right: 2px;
        }
        QSpinBox::down-button:hover {
            background-color: rgba(102, 204, 255, 200);
        }
    )");
    
    connect(minEnhancementLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &StarryCard::onMinEnhancementLevelChanged);
    
    // 最高强化等级设置
    QLabel* maxLevelLabel = new QLabel("最高强化星级:");
    maxLevelLabel->setStyleSheet(R"(
        QLabel {
            color: #003D7A;
            font-weight: bold;
            font-size: 14px;
            background-color: rgba(125, 197, 255, 100);
            border-radius: 4px;
            padding: 5px 8px;
        }
    )");
    
    maxEnhancementLevelSpinBox = new QSpinBox();
    maxEnhancementLevelSpinBox->setMinimum(1);
    maxEnhancementLevelSpinBox->setMaximum(14);
    maxEnhancementLevelSpinBox->setValue(14); // 默认为14级
    maxEnhancementLevelSpinBox->setSuffix(" 星");
    maxEnhancementLevelSpinBox->setStyleSheet(R"(
        QSpinBox {
            background-color: rgba(255, 255, 255, 220);
            border: 2px solid rgba(102, 204, 255, 150);
            border-radius: 6px;
            padding: 4px 8px;
            color: #003D7A;
            font-weight: bold;
            font-size: 13px;
            min-width: 80px;
        }
        QSpinBox:hover {
            background-color: rgba(102, 204, 255, 100);
            border: 2px solid rgba(102, 204, 255, 200);
        }
        QSpinBox:focus {
            background-color: rgba(255, 255, 255, 255);
            border: 2px solid rgba(33, 150, 243, 200);
        }
        QSpinBox::up-button {
            image: url(:/images/icons/uparrow_1f.svg);
            background-color: rgba(102, 204, 255, 150);
            border: 1px solid rgba(102, 204, 255, 200);
            border-radius: 3px;
            width: 16px;
            margin-right: 2px;
        }
        QSpinBox::up-button:hover {
            background-color: rgba(102, 204, 255, 200);
        }
        QSpinBox::down-button {
            image: url(:/images/icons/downarrow_1f.svg);
            background-color: rgba(102, 204, 255, 150);
            border: 1px solid rgba(102, 204, 255, 200);
            border-radius: 3px;
            width: 16px;
            margin-right: 2px;
        }
        QSpinBox::down-button:hover {
            background-color: rgba(102, 204, 255, 200);
        }
    )");
    
    connect(maxEnhancementLevelSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &StarryCard::onMaxEnhancementLevelChanged);
    
    // 初始化时同步最低等级SpinBox的最大值
    if (maxEnhancementLevelSpinBox) {
        minEnhancementLevelSpinBox->setMaximum(maxEnhancementLevelSpinBox->value());
        maxEnhancementLevelSpinBox->setMinimum(minEnhancementLevelSpinBox->value());
    }
    
    // 组装水平布局：最低等级 - 最高等级 - 弹性空间
    levelSettingsLayout->addWidget(minLevelLabel);
    levelSettingsLayout->addWidget(minEnhancementLevelSpinBox);
    levelSettingsLayout->addSpacing(30); // 添加间距分隔两个设置区域
    levelSettingsLayout->addWidget(maxLevelLabel);
    levelSettingsLayout->addWidget(maxEnhancementLevelSpinBox);
    levelSettingsLayout->addStretch(); // 右侧弹性空间
    
    layout->addLayout(levelSettingsLayout);
    
    // 添加一些垂直间距
    layout->addSpacing(5);
    
    // 添加按钮布局
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(5);
    btnLayout->setContentsMargins(0, 0, 0, 5);
    
    // 加载配置按钮
    QPushButton* loadBtn = new QPushButton("加载配置");
    loadBtn->setFixedSize(120, 35);
    loadBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
    )");
    connect(loadBtn, &QPushButton::clicked, this, &StarryCard::loadEnhancementConfigFromFile);
    
    // 保存配置按钮
    QPushButton* saveBtn = new QPushButton("保存配置");
    saveBtn->setFixedSize(120, 35);
    saveBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
    )");
    connect(saveBtn, &QPushButton::clicked, this, &StarryCard::saveEnhancementConfig);
    
    btnLayout->addStretch();
    btnLayout->addWidget(loadBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    // 加载现有配置
    loadEnhancementConfig();
    
    return page;
}

int StarryCard::getMinSubCardLevel(int enhancementLevel)
{
    if (enhancementLevel <= 2) {
        return 0;
    } else if (enhancementLevel == 3) {
        return 1;
    } else {
        return qMax(0, enhancementLevel - 2);
    }
}

int StarryCard::getMaxSubCardLevel(int enhancementLevel)
{
    return enhancementLevel;
}

int StarryCard::getMaxEnhancementLevel() const
{
    if (maxEnhancementLevelSpinBox) {
        return maxEnhancementLevelSpinBox->value();
    }
    return 14; // 默认最高等级为14
}

int StarryCard::getMinEnhancementLevel() const
{
    if (minEnhancementLevelSpinBox) {
        return minEnhancementLevelSpinBox->value();
    }
    return 1; // 默认最低等级为1
}

void StarryCard::onEnhancementConfigChanged()
{
    // 延迟保存配置，避免频繁I/O操作
    scheduleConfigSave();
}

void StarryCard::onMaxEnhancementLevelChanged()
{
    if (!maxEnhancementLevelSpinBox || !minEnhancementLevelSpinBox) return;
    
    int maxLevel = maxEnhancementLevelSpinBox->value();
    int minLevel = minEnhancementLevelSpinBox->value();
    
    // 确保最高等级不小于最低等级
    if (maxLevel < minLevel) {
        minEnhancementLevelSpinBox->setValue(maxLevel);
        minLevel = maxLevel;
    }
    
    // 更新最低强化等级SpinBox的最大值
    minEnhancementLevelSpinBox->setMaximum(maxLevel);
    
    // 更新表格UI状态
    updateEnhancementTableUI();
    
    // 延迟保存配置，避免频繁I/O操作
    scheduleConfigSave();
    
    addLog(QString("最高强化等级已设置为: %1级").arg(maxLevel), LogType::Success);
}

void StarryCard::onMinEnhancementLevelChanged()
{
    if (!minEnhancementLevelSpinBox || !maxEnhancementLevelSpinBox) return;
    
    int minLevel = minEnhancementLevelSpinBox->value();
    int maxLevel = maxEnhancementLevelSpinBox->value();
    
    // 确保最低等级不超过最高等级
    if (minLevel > maxLevel) {
        maxEnhancementLevelSpinBox->setValue(minLevel);
        maxLevel = minLevel;
    }
    
    // 更新最高强化等级SpinBox的最小值
    maxEnhancementLevelSpinBox->setMinimum(minLevel);
    
    // 更新表格UI状态
    updateEnhancementTableUI();
    
    // 延迟保存配置，避免频繁I/O操作
    scheduleConfigSave();
    
    addLog(QString("最低强化等级已设置为: %1级").arg(minLevel), LogType::Info);
}

void StarryCard::scheduleConfigSave()
{
    // 重启定时器，如果用户快速点击，会取消之前的保存操作
    if (configSaveTimer && configSaveTimer->isActive()) {
        configSaveTimer->stop();
    }
    if (configSaveTimer) {
        configSaveTimer->start();
    }
}

void StarryCard::onConfigSaveTimeout()
{
    // 定时器超时，真正执行保存操作
    saveEnhancementConfig();
}

void StarryCard::scheduleSpiceConfigSave()
{
    // 重启定时器，如果用户快速更改，会取消之前的保存操作
    if (spiceSaveTimer && spiceSaveTimer->isActive()) {
        spiceSaveTimer->stop();
    }
    if (spiceSaveTimer) {
        spiceSaveTimer->start();
    }
}

void StarryCard::onSpiceSaveTimeout()
{
    // 定时器超时，真正执行保存操作
    saveSpiceConfig();
    // 只在延迟保存完成时显示一次日志
    addLog("香料配置已保存", LogType::Info);
}

void StarryCard::updateEnhancementTableUI()
{
    if (!enhancementTable || !minEnhancementLevelSpinBox || !maxEnhancementLevelSpinBox) return;
    
    int minLevel = minEnhancementLevelSpinBox->value();
    int maxLevel = maxEnhancementLevelSpinBox->value();
    
    // 批量更新表格显示，禁用低于最低等级和超过最高等级的行
    for (int row = 0; row < 14; ++row) {
        bool enabled = (row + 1) >= minLevel && (row + 1) <= maxLevel;
        
        // 设置行的启用状态和视觉效果
        for (int col = 0; col < enhancementTable->columnCount(); ++col) {
            QWidget* widget = enhancementTable->cellWidget(row, col);
            if (widget) {
                widget->setEnabled(enabled);
                
                // 优化样式表更新 - 只在状态真正改变时更新
                QString currentStyle = widget->styleSheet();
                bool isCurrentlyDisabled = currentStyle.contains("color: #CCCCCC;");
                
                if (enabled && isCurrentlyDisabled) {
                    // 从禁用状态恢复到启用状态
                    widget->setStyleSheet(currentStyle.replace("color: #CCCCCC;", "color: #003D7A;"));
                } else if (!enabled && !isCurrentlyDisabled) {
                    // 从启用状态改为禁用状态
                    widget->setStyleSheet(currentStyle.replace("color: #003D7A;", "color: #CCCCCC;"));
                }
            }
        }
    }
}

void StarryCard::saveEnhancementConfig()
{
    if (!enhancementTable) return;
    
    // 读取现有配置，保留主卡/副卡设置
    QJsonObject config;
    QFile file(enhancementConfigPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            config = doc.object();
        }
        file.close();
    }
    
    for (int row = 0; row < 14; ++row) {
        QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
        QJsonObject levelConfig;
        
        // 如果已存在配置，先获取现有配置（特别是主卡/副卡配置）
        if (config.contains(levelKey)) {
            levelConfig = config[levelKey].toObject();
        }
        
        // 获取副卡配置
        QComboBox* subCard1 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 1));
        QComboBox* subCard2 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 2));
        QComboBox* subCard3 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 3));
        
        if (subCard1) {
            levelConfig["subcard1"] = subCard1->currentIndex() + getMinSubCardLevel(row);
        }
        if (subCard2) {
            levelConfig["subcard2"] = subCard2->currentIndex() == 0 ? -1 : 
                                      (subCard2->currentIndex() - 1 + getMinSubCardLevel(row));
        }
        if (subCard3) {
            levelConfig["subcard3"] = subCard3->currentIndex() == 0 ? -1 : 
                                      (subCard3->currentIndex() - 1 + getMinSubCardLevel(row));
        }
        
        // 获取四叶草配置
        QComboBox* clover = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 4));
        if (clover) {
            levelConfig["clover"] = clover->currentText();
        }
        
        // 获取四叶草状态配置
        QWidget* cloverStateWidget = enhancementTable->cellWidget(row, 5);
        QHBoxLayout* cloverStateLayout = qobject_cast<QHBoxLayout*>(cloverStateWidget->layout());
        QCheckBox* bindCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(0)->widget());
        QCheckBox* unboundCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(1)->widget());
        
        if (bindCheck && unboundCheck) {
            levelConfig["clover_bound"] = bindCheck->isChecked();
            levelConfig["clover_unbound"] = unboundCheck->isChecked();
        }
        
        // 保留主卡/副卡配置（如果不存在则设置默认值）
        if (!levelConfig.contains("main_card_type")) {
            levelConfig["main_card_type"] = "无";
        }
        if (!levelConfig.contains("main_card_bound")) {
            levelConfig["main_card_bound"] = false;
        }
        if (!levelConfig.contains("main_card_unbound")) {
            levelConfig["main_card_unbound"] = false;
        }
        if (!levelConfig.contains("sub_card_type")) {
            levelConfig["sub_card_type"] = "无";
        }
        if (!levelConfig.contains("sub_card_bound")) {
            levelConfig["sub_card_bound"] = false;
        }
        if (!levelConfig.contains("sub_card_unbound")) {
            levelConfig["sub_card_unbound"] = false;
        }
        
        config[levelKey] = levelConfig;
    }
    
    // 保存最高强化等级设置
    if (maxEnhancementLevelSpinBox) {
        config["max_enhancement_level"] = maxEnhancementLevelSpinBox->value();
    }
    
    // 保存最低强化等级设置
    if (minEnhancementLevelSpinBox) {
        config["min_enhancement_level"] = minEnhancementLevelSpinBox->value();
    }
    
    // 保存到文件
    QJsonDocument doc(config);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        addLog("强化方案配置已保存", LogType::Success);
    } else {
        addLog("保存强化方案配置失败", LogType::Error);
    }
}

void StarryCard::loadEnhancementConfig()
{
    if (!enhancementTable) return;
    
    QFile file(enhancementConfigPath);
    if (!file.open(QIODevice::ReadOnly)) {
        addLog("未找到强化方案配置文件，使用默认配置", LogType::Info);
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        addLog("强化方案配置文件格式错误", LogType::Error);
        return;
    }
    
    QJsonObject config = doc.object();
    
    // 加载配置时阻止信号发射，避免触发保存
    for (int row = 0; row < 14; ++row) {
        QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
        if (!config.contains(levelKey)) continue;
        
        QJsonObject levelConfig = config[levelKey].toObject();
        
        // 设置副卡配置，阻止信号发射
        QComboBox* subCard1 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 1));
        if (subCard1 && levelConfig.contains("subcard1")) {
            int value = levelConfig["subcard1"].toInt();
            int index = value - getMinSubCardLevel(row);
            if (index >= 0 && index < subCard1->count()) {
                subCard1->blockSignals(true);
                subCard1->setCurrentIndex(index);
                subCard1->blockSignals(false);
            }
        }
        
        QComboBox* subCard2 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 2));
        if (subCard2 && levelConfig.contains("subcard2")) {
            int value = levelConfig["subcard2"].toInt();
            subCard2->blockSignals(true);
            if (value == -1) {
                subCard2->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard2->count()) {
                    subCard2->setCurrentIndex(index);
                }
            }
            subCard2->blockSignals(false);
        }
        
        QComboBox* subCard3 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 3));
        if (subCard3 && levelConfig.contains("subcard3")) {
            int value = levelConfig["subcard3"].toInt();
            subCard3->blockSignals(true);
            if (value == -1) {
                subCard3->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard3->count()) {
                    subCard3->setCurrentIndex(index);
                }
            }
            subCard3->blockSignals(false);
        }
        
        // 设置四叶草配置
        QComboBox* clover = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 4));
        if (clover && levelConfig.contains("clover")) {
            QString value = levelConfig["clover"].toString();
            int index = clover->findText(value);
            if (index >= 0) {
                clover->blockSignals(true);
                clover->setCurrentIndex(index);
                clover->blockSignals(false);
            }
        }
        
        // 设置四叶草状态配置
        QWidget* cloverStateWidget = enhancementTable->cellWidget(row, 5);
        QHBoxLayout* cloverStateLayout = qobject_cast<QHBoxLayout*>(cloverStateWidget->layout());
        QCheckBox* bindCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(0)->widget());
        QCheckBox* unboundCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(1)->widget());
        
        if (bindCheck && unboundCheck) {
            if (levelConfig.contains("clover_bound")) {
                bindCheck->blockSignals(true);
                bindCheck->setChecked(levelConfig["clover_bound"].toBool());
                bindCheck->blockSignals(false);
            }
            if (levelConfig.contains("clover_unbound")) {
                unboundCheck->blockSignals(true);
                unboundCheck->setChecked(levelConfig["clover_unbound"].toBool());
                unboundCheck->blockSignals(false);
            }
        }
        
        // 卡片设置配置现在通过对话框处理，不再需要在这里加载
    }
    
    // 加载最高强化等级设置
    if (maxEnhancementLevelSpinBox && config.contains("max_enhancement_level")) {
        int maxLevel = config["max_enhancement_level"].toInt();
        if (maxLevel >= 1 && maxLevel <= 14) {
            maxEnhancementLevelSpinBox->blockSignals(true);
            maxEnhancementLevelSpinBox->setValue(maxLevel);
            maxEnhancementLevelSpinBox->blockSignals(false);
        }
    }
    
    // 加载最低强化等级设置
    if (minEnhancementLevelSpinBox && config.contains("min_enhancement_level")) {
        int minLevel = config["min_enhancement_level"].toInt();
        if (minLevel >= 1 && minLevel <= 14) {
            minEnhancementLevelSpinBox->blockSignals(true);
            minEnhancementLevelSpinBox->setValue(minLevel);
            minEnhancementLevelSpinBox->blockSignals(false);
        }
    }
    
    // 触发槽函数以更新表格显示状态
    if (maxEnhancementLevelSpinBox) {
        onMaxEnhancementLevelChanged();
    }
    
    addLog("强化方案配置已加载", LogType::Success);
}

void StarryCard::loadEnhancementConfigFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
        "选择强化方案配置文件", 
        "", 
        "JSON配置文件 (*.json);;所有文件 (*.*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        addLog("无法读取配置文件: " + fileName, LogType::Error);
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        addLog("配置文件格式错误", LogType::Error);
        return;
    }
    
    QJsonObject config = doc.object();
    
    if (!enhancementTable) return;
    
    // 验证配置文件是否包含必要的配置
    bool hasValidConfig = false;
    for (int row = 0; row < 14; ++row) {
        QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
        if (config.contains(levelKey)) {
            hasValidConfig = true;
            break;
        }
    }
    
    if (!hasValidConfig) {
        addLog("配置文件不包含有效的强化配置", LogType::Error);
        return;
    }
    
    // 加载配置到表格，阻止信号发射避免触发保存
    for (int row = 0; row < 14; ++row) {
        QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
        if (!config.contains(levelKey)) continue;
        
        QJsonObject levelConfig = config[levelKey].toObject();
        
        // 设置副卡配置，阻止信号发射
        QComboBox* subCard1 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 1));
        if (subCard1 && levelConfig.contains("subcard1")) {
            int value = levelConfig["subcard1"].toInt();
            int index = value - getMinSubCardLevel(row);
            if (index >= 0 && index < subCard1->count()) {
                subCard1->blockSignals(true);
                subCard1->setCurrentIndex(index);
                subCard1->blockSignals(false);
            }
        }
        
        QComboBox* subCard2 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 2));
        if (subCard2 && levelConfig.contains("subcard2")) {
            int value = levelConfig["subcard2"].toInt();
            subCard2->blockSignals(true);
            if (value == -1) {
                subCard2->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard2->count()) {
                    subCard2->setCurrentIndex(index);
                }
            }
            subCard2->blockSignals(false);
        }
        
        QComboBox* subCard3 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 3));
        if (subCard3 && levelConfig.contains("subcard3")) {
            int value = levelConfig["subcard3"].toInt();
            subCard3->blockSignals(true);
            if (value == -1) {
                subCard3->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard3->count()) {
                    subCard3->setCurrentIndex(index);
                }
            }
            subCard3->blockSignals(false);
        }
        
        // 设置四叶草配置
        QComboBox* clover = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 4));
        if (clover && levelConfig.contains("clover")) {
            QString value = levelConfig["clover"].toString();
            int index = clover->findText(value);
            if (index >= 0) {
                clover->blockSignals(true);
                clover->setCurrentIndex(index);
                clover->blockSignals(false);
            }
        }
        
        // 设置四叶草状态配置
        QWidget* cloverStateWidget = enhancementTable->cellWidget(row, 5);
        QHBoxLayout* cloverStateLayout = qobject_cast<QHBoxLayout*>(cloverStateWidget->layout());
        QCheckBox* bindCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(0)->widget());
        QCheckBox* unboundCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(1)->widget());
        
        if (bindCheck && unboundCheck) {
            if (levelConfig.contains("clover_bound")) {
                bindCheck->blockSignals(true);
                bindCheck->setChecked(levelConfig["clover_bound"].toBool());
                bindCheck->blockSignals(false);
            }
            if (levelConfig.contains("clover_unbound")) {
                unboundCheck->blockSignals(true);
                unboundCheck->setChecked(levelConfig["clover_unbound"].toBool());
                unboundCheck->blockSignals(false);
            }
        }
        
        // 卡片设置配置现在通过对话框处理，不再需要在这里加载
    }
    
    // 加载最高强化等级设置
    if (maxEnhancementLevelSpinBox && config.contains("max_enhancement_level")) {
        int maxLevel = config["max_enhancement_level"].toInt();
        if (maxLevel >= 1 && maxLevel <= 14) {
            maxEnhancementLevelSpinBox->blockSignals(true);
            maxEnhancementLevelSpinBox->setValue(maxLevel);
            maxEnhancementLevelSpinBox->blockSignals(false);
        }
    }
    
    // 加载最低强化等级设置
    if (minEnhancementLevelSpinBox && config.contains("min_enhancement_level")) {
        int minLevel = config["min_enhancement_level"].toInt();
        if (minLevel >= 1 && minLevel <= 14) {
            minEnhancementLevelSpinBox->blockSignals(true);
            minEnhancementLevelSpinBox->setValue(minLevel);
            minEnhancementLevelSpinBox->blockSignals(false);
        }
    }
    
    // 触发槽函数以更新表格显示状态
    if (maxEnhancementLevelSpinBox) {
        onMaxEnhancementLevelChanged();
    }
    
    addLog("从文件加载配置成功: " + QFileInfo(fileName).fileName(), LogType::Success);
    
    // 延迟保存当前配置以覆盖默认配置，避免频繁I/O操作
    scheduleConfigSave();
}

QStringList StarryCard::getCardTypes() const
{
    QStringList cardTypes;
    if (cardRecognizer) {
        // 从卡片识别器获取已加载的卡片类型
        cardTypes = cardRecognizer->getRegisteredCards();
    }
    
    // 如果没有加载到卡片类型
    if (cardTypes.isEmpty()) {
        qDebug() << "没有找到任何卡片类型";
    }
    
    // 按卡片前缀排序：好卡 -> 中卡 -> 差卡 -> 其他
    QStringList sortedCardTypes;
    QStringList goodCards;   // "好卡："前缀
    QStringList mediumCards; // "中卡："前缀  
    QStringList badCards;    // "差卡："前缀
    QStringList otherCards;  // 其他没有这些前缀的卡片
    
    // 分类卡片
    for (const QString &card : cardTypes) {
        if (card.startsWith("好卡：")) {
            goodCards.append(card);
        } else if (card.startsWith("中卡：")) {
            mediumCards.append(card);
        } else if (card.startsWith("差卡：")) {
            badCards.append(card);
        } else {
            otherCards.append(card);
        }
    }
    
    // 各组内部按字母顺序排序
    goodCards.sort();
    mediumCards.sort();
    badCards.sort();
    otherCards.sort();
    
    // 按优先级顺序合并：好卡 -> 中卡 -> 差卡 -> 其他
    sortedCardTypes.append(goodCards);
    sortedCardTypes.append(mediumCards);
    sortedCardTypes.append(badCards);
    sortedCardTypes.append(otherCards);
    
    return sortedCardTypes;
}

QStringList StarryCard::getRequiredCardTypesFromConfig()
{
    QStringList requiredCards;
    QSet<QString> uniqueCards; // 使用QSet避免重复
    
    QFile file(enhancementConfigPath);
    if (!file.open(QIODevice::ReadOnly)) {
        addLog("无法读取强化配置文件，返回空的卡片类型列表", LogType::Warning);
        return requiredCards;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    file.close();
    
    if (doc.isNull() || !doc.isObject()) {
        addLog("强化配置文件格式错误，返回空的卡片类型列表", LogType::Error);
        return requiredCards;
    }
    
    QJsonObject config = doc.object();
    int maxLevel = config["max_enhancement_level"].toInt();
    int minLevel = config["min_enhancement_level"].toInt();
    
    // 遍历所有等级的配置
    for (int row = minLevel; row <= maxLevel; ++row) {
        QString levelKey = QString("%1-%2").arg(row - 1).arg(row);
        if (!config.contains(levelKey)) continue;
        
        QJsonObject levelConfig = config[levelKey].toObject();
        
        // 获取主卡类型
        if (levelConfig.contains("main_card_type")) {
            QString mainCardType = levelConfig["main_card_type"].toString();
            if (!mainCardType.isEmpty() && mainCardType != "无") {
                uniqueCards.insert(mainCardType);
            }
        }
        
        // 获取副卡类型
        if (levelConfig.contains("sub_card_type")) {
            QString subCardType = levelConfig["sub_card_type"].toString();
            if (!subCardType.isEmpty() && subCardType != "无") {
                uniqueCards.insert(subCardType);
            }
        }
    }
    
    // 转换为QStringList
    requiredCards = uniqueCards.values();
    
    // 记录找到的卡片类型
    if (!requiredCards.isEmpty()) {
        addLog(QString("从配置文件中提取到 %1 种需要的卡片类型: %2")
               .arg(requiredCards.size())
               .arg(requiredCards.join(", ")), LogType::Info);
    } else {
        addLog("配置文件中未找到任何有效的卡片类型", LogType::Warning);
    }
    
    return requiredCards;
}

void StarryCard::onCardSettingButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    int row = button->property("row").toInt();
    QStringList cardTypes = getCardTypes();
    
    CardSettingDialog dialog(row, cardTypes, this);
    
    // 从JSON加载当前配置
    QFile file(enhancementConfigPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject config = doc.object();
            QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
            if (config.contains(levelKey)) {
                QJsonObject levelConfig = config[levelKey].toObject();
                
                // 加载主卡配置
                if (levelConfig.contains("main_card_type")) {
                    dialog.setMainCardType(levelConfig["main_card_type"].toString());
                }
                if (levelConfig.contains("main_card_bound") && levelConfig.contains("main_card_unbound")) {
                    dialog.setMainCardBind(levelConfig["main_card_bound"].toBool(), 
                                         levelConfig["main_card_unbound"].toBool());
                }
                
                // 加载副卡配置
                if (levelConfig.contains("sub_card_type")) {
                    dialog.setSubCardType(levelConfig["sub_card_type"].toString());
                }
                if (levelConfig.contains("sub_card_bound") && levelConfig.contains("sub_card_unbound")) {
                    dialog.setSubCardBind(levelConfig["sub_card_bound"].toBool(), 
                                        levelConfig["sub_card_unbound"].toBool());
                }
            }
        }
    }
    
    // 连接信号
    connect(&dialog, &CardSettingDialog::applyMainCardToAll,
            this, &StarryCard::onApplyMainCardToAll);
    connect(&dialog, &CardSettingDialog::applySubCardToAll,
            this, &StarryCard::onApplySubCardToAll);
    
    // 设置对话框位置在主窗口右侧
    QPoint mainWindowPos = this->pos();
    QSize mainWindowSize = this->size();
    QSize dialogSize = dialog.size();
    
    // 计算右侧位置，留一点间距
    int dialogX = mainWindowPos.x() + mainWindowSize.width() - 180;
    int dialogY = mainWindowPos.y() + (mainWindowSize.height() - dialogSize.height()) / 2;
    
    // 确保对话框不会超出屏幕范围
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        if (dialogX + dialogSize.width() > screenGeometry.right()) {
            // 如果右侧空间不够，放在主窗口左侧
            dialogX = mainWindowPos.x() - dialogSize.width() - 2;
        }
        if (dialogY < screenGeometry.top()) {
            dialogY = screenGeometry.top();
        } else if (dialogY + dialogSize.height() > screenGeometry.bottom()) {
            dialogY = screenGeometry.bottom() - dialogSize.height();
        }
    }
    
    dialog.move(dialogX, dialogY);
    
    if (dialog.exec() == QDialog::Accepted) {
        // 保存当前行的配置
        saveCardSettingForRow(row, dialog.getMainCardType(), dialog.getSubCardType(),
                             dialog.getMainCardBind(), dialog.getMainCardUnbound(),
                             dialog.getSubCardBind(), dialog.getSubCardUnbound());
    }
}

void StarryCard::onApplyMainCardToAll(int row, const QString& cardType, bool bind, bool unbound)
{
    // 将主卡配置应用到所有等级
    QFile file(enhancementConfigPath);
    QJsonObject config;
    
    // 读取现有配置
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            config = doc.object();
        }
        file.close();
    }
    
    // 更新所有等级的主卡配置
    for (int i = 0; i < 14; ++i) {
        QString levelKey = QString("%1-%2").arg(i).arg(i + 1);
        QJsonObject levelConfig;
        if (config.contains(levelKey)) {
            levelConfig = config[levelKey].toObject();
        }
        
        levelConfig["main_card_type"] = cardType;
        levelConfig["main_card_bound"] = bind;
        levelConfig["main_card_unbound"] = unbound;
        
        config[levelKey] = levelConfig;
    }
    
    // 保存配置
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        file.write(doc.toJson());
        file.close();
        addLog("主卡配置已应用到所有等级", LogType::Success);
    }
}

void StarryCard::onApplySubCardToAll(int row, const QString& cardType, bool bind, bool unbound)
{
    // 将副卡配置应用到所有等级
    QFile file(enhancementConfigPath);
    QJsonObject config;
    
    // 读取现有配置
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            config = doc.object();
        }
        file.close();
    }
    
    // 更新所有等级的副卡配置
    for (int i = 0; i < 14; ++i) {
        QString levelKey = QString("%1-%2").arg(i).arg(i + 1);
        QJsonObject levelConfig;
        if (config.contains(levelKey)) {
            levelConfig = config[levelKey].toObject();
        }
        
        levelConfig["sub_card_type"] = cardType;
        levelConfig["sub_card_bound"] = bind;
        levelConfig["sub_card_unbound"] = unbound;
        
        config[levelKey] = levelConfig;
    }
    
    // 保存配置
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        file.write(doc.toJson());
        file.close();
        addLog("副卡配置已应用到所有等级", LogType::Success);
    }
}

void StarryCard::saveCardSettingForRow(int row, const QString& mainCardType, const QString& subCardType,
                                    bool mainBind, bool mainUnbound, bool subBind, bool subUnbound)
{
    QFile file(enhancementConfigPath);
    QJsonObject config;
    
    // 读取现有配置
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            config = doc.object();
        }
        file.close();
    }
    
    // 更新指定行的配置
    QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
    QJsonObject levelConfig;
    if (config.contains(levelKey)) {
        levelConfig = config[levelKey].toObject();
    }
    
    levelConfig["main_card_type"] = mainCardType;
    levelConfig["main_card_bound"] = mainBind;
    levelConfig["main_card_unbound"] = mainUnbound;
    levelConfig["sub_card_type"] = subCardType;
    levelConfig["sub_card_bound"] = subBind;
    levelConfig["sub_card_unbound"] = subUnbound;
    
    config[levelKey] = levelConfig;
    
    // 保存配置
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        file.write(doc.toJson());
        file.close();
        addLog(QString("等级%1-%2的卡片设置已保存").arg(row).arg(row + 1), LogType::Success);
    }
}

// CardSettingDialog 实现
CardSettingDialog::CardSettingDialog(int row, const QStringList& cardTypes, QWidget *parent)
    : QDialog(parent), m_row(row), m_configChanged(false)
{
    setWindowTitle(QString("卡片设置 - 等级%1→%2").arg(row).arg(row + 1));
    setFixedSize(400, 350);
    setModal(true);
    
    // 设置样式
    setStyleSheet(R"(
        QDialog {
            background-color: rgba(255, 255, 255, 240);
            border: 2px solid rgba(102, 204, 255, 200);
            border-radius: 10px;
        }
        QGroupBox {
            font-weight: bold;
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 150);
            border-radius: 8px;
            margin: 5px 0;
            padding-top: 15px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 10px 0 10px;
            background-color: rgba(255, 255, 255, 200);
        }
        QComboBox {
            background-color: rgba(255, 255, 255, 220);
            border: 1px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 3px 8px;
            color: #003D7A;
            font-size: 11px;
        }
        QComboBox:hover {
            background-color: rgba(102, 204, 255, 100);
        }
        QCheckBox {
            color: #003D7A;
            font-size: 11px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }
        QCheckBox::indicator:unchecked {
            border: 2px solid rgba(102, 204, 255, 150);
            background-color: rgba(255, 255, 255, 200);
            border-radius: 3px;
        }
        QCheckBox::indicator:checked {
            border: 2px solid rgba(102, 204, 255, 150);
            image: url(:/images/icons/勾选.svg);
            background-color: rgba(102, 204, 255, 200);
            border-radius: 3px;
        }
        QPushButton {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(102, 204, 255, 255);
            border-radius: 6px;
            font-weight: bold;
            font-size: 11px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: rgba(102, 204, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
        QPushButton:disabled {
            background-color: rgba(200, 200, 200, 150);
            color: rgba(100, 100, 100, 150);
            border: 2px solid rgba(200, 200, 200, 200);
        }
    )");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 主卡配置组
    QGroupBox* mainCardGroup = new QGroupBox("主卡配置");
    QVBoxLayout* mainCardLayout = new QVBoxLayout(mainCardGroup);
    mainCardLayout->setSpacing(8);
    
    // 主卡类型
    QHBoxLayout* mainTypeLayout = new QHBoxLayout();
    mainTypeLayout->addWidget(new QLabel("卡片类型:"));
    m_mainCardCombo = new QComboBox();
    m_mainCardCombo->addItems(cardTypes);
    mainTypeLayout->addWidget(m_mainCardCombo);
    mainCardLayout->addLayout(mainTypeLayout);
    
    // 主卡绑定状态
    QHBoxLayout* mainBindLayout = new QHBoxLayout();
    mainBindLayout->addWidget(new QLabel("绑定状态:"));
    m_mainBindCheck = new QCheckBox("绑定");
    m_mainUnboundCheck = new QCheckBox("不绑");
    mainBindLayout->addWidget(m_mainBindCheck);
    mainBindLayout->addWidget(m_mainUnboundCheck);
    mainBindLayout->addStretch();
    mainCardLayout->addLayout(mainBindLayout);
    
    // 主卡应用按钮
    m_applyMainBtn = new QPushButton("应用到所有等级");
    mainCardLayout->addWidget(m_applyMainBtn);
    
    mainLayout->addWidget(mainCardGroup);
    
    // 副卡配置组
    QGroupBox* subCardGroup = new QGroupBox("副卡配置");
    QVBoxLayout* subCardLayout = new QVBoxLayout(subCardGroup);
    subCardLayout->setSpacing(8);
    
    // 副卡类型
    QHBoxLayout* subTypeLayout = new QHBoxLayout();
    subTypeLayout->addWidget(new QLabel("卡片类型:"));
    m_subCardCombo = new QComboBox();
    m_subCardCombo->addItem("无");
    m_subCardCombo->addItems(cardTypes);
    subTypeLayout->addWidget(m_subCardCombo);
    subCardLayout->addLayout(subTypeLayout);
    
    // 副卡绑定状态
    QHBoxLayout* subBindLayout = new QHBoxLayout();
    subBindLayout->addWidget(new QLabel("绑定状态:"));
    m_subBindCheck = new QCheckBox("绑定");
    m_subUnboundCheck = new QCheckBox("不绑");
    subBindLayout->addWidget(m_subBindCheck);
    subBindLayout->addWidget(m_subUnboundCheck);
    subBindLayout->addStretch();
    subCardLayout->addLayout(subBindLayout);
    
    // 副卡应用按钮
    m_applySubBtn = new QPushButton("应用到所有等级");
    subCardLayout->addWidget(m_applySubBtn);
    
    mainLayout->addWidget(subCardGroup);
    
    // 对话框按钮
    QHBoxLayout* dialogBtnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定");
    QPushButton* cancelBtn = new QPushButton("取消");
    okBtn->setDefault(true);
    
    dialogBtnLayout->addStretch();
    dialogBtnLayout->addWidget(okBtn);
    dialogBtnLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(dialogBtnLayout);
    
    // 连接信号
    connect(m_mainCardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CardSettingDialog::onConfigChanged);
    connect(m_mainBindCheck, &QCheckBox::checkStateChanged, this, &CardSettingDialog::onConfigChanged);
    connect(m_mainUnboundCheck, &QCheckBox::checkStateChanged, this, &CardSettingDialog::onConfigChanged);
    connect(m_subCardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CardSettingDialog::onConfigChanged);
    connect(m_subBindCheck, &QCheckBox::checkStateChanged, this, &CardSettingDialog::onConfigChanged);
    connect(m_subUnboundCheck, &QCheckBox::checkStateChanged, this, &CardSettingDialog::onConfigChanged);
    
    connect(m_applyMainBtn, &QPushButton::clicked, this, &CardSettingDialog::onApplyMainCardToAll);
    connect(m_applySubBtn, &QPushButton::clicked, this, &CardSettingDialog::onApplySubCardToAll);
    
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    // 初始状态下禁用应用按钮
    m_applyMainBtn->setEnabled(false);
    m_applySubBtn->setEnabled(false);
}

QString CardSettingDialog::getMainCardType() const
{
    return m_mainCardCombo->currentText();
}

QString CardSettingDialog::getSubCardType() const
{
    return m_subCardCombo->currentText();
}

bool CardSettingDialog::getMainCardBind() const
{
    return m_mainBindCheck->isChecked();
}

bool CardSettingDialog::getMainCardUnbound() const
{
    return m_mainUnboundCheck->isChecked();
}

bool CardSettingDialog::getSubCardBind() const
{
    return m_subBindCheck->isChecked();
}

bool CardSettingDialog::getSubCardUnbound() const
{
    return m_subUnboundCheck->isChecked();
}

void CardSettingDialog::setMainCardType(const QString& type)
{
    int index = m_mainCardCombo->findText(type);
    if (index >= 0) {
        m_mainCardCombo->setCurrentIndex(index);
    }
}

void CardSettingDialog::setSubCardType(const QString& type)
{
    int index = m_subCardCombo->findText(type);
    if (index >= 0) {
        m_subCardCombo->setCurrentIndex(index);
    }
}

void CardSettingDialog::setMainCardBind(bool bind, bool unbound)
{
    m_mainBindCheck->setChecked(bind);
    m_mainUnboundCheck->setChecked(unbound);
}

void CardSettingDialog::setSubCardBind(bool bind, bool unbound)
{
    m_subBindCheck->setChecked(bind);
    m_subUnboundCheck->setChecked(unbound);
}

void CardSettingDialog::onApplyMainCardToAll()
{
    emit applyMainCardToAll(m_row, getMainCardType(), getMainCardBind(), getMainCardUnbound());
    m_applyMainBtn->setEnabled(false);
}

void CardSettingDialog::onApplySubCardToAll()
{
    emit applySubCardToAll(m_row, getSubCardType(), getSubCardBind(), getSubCardUnbound());
    m_applySubBtn->setEnabled(false);
}

void CardSettingDialog::onConfigChanged()
{
    m_configChanged = true;
    m_applyMainBtn->setEnabled(true);
    m_applySubBtn->setEnabled(true);
}

// ================== 四叶草识别功能实现 ==================

bool StarryCard::loadCloverTemplates()
{
    QStringList cloverTypes = {
        "1级", "2级", "3级", "4级", "5级", "6级",
        "SS", "SSS", "SSR", "S", "蛇草"
    };
    
    // QStringList cloverFiles = {
    //     "1级四叶草.png", "2级四叶草.png", "3级四叶草.png", "4级四叶草.png", 
    //     "5级四叶草.png", "6级四叶草.png", "SS四叶草.png", "SSS四叶草.png", 
    //     "SSR四叶草.png", "超能四叶草.png"
    // };
    
    cloverTemplateHashes.clear();
    
    for (int i = 0; i < cloverTypes.size(); ++i) {
        QString filePath = QString(":/images/clover/%1").arg(cloverTypes[i]);
        QImage template_image(filePath);
        
        if (template_image.isNull()) {
            qDebug() << "无法加载四叶草模板:" << cloverTypes[i] << "路径:" << filePath;
            continue;
        }
        
        // 保存模板图像和计算颜色直方图
        QRect roi(4, 4, 38, 24);
        cloverTemplateImages[cloverTypes[i]] = template_image;
        QVector<double> histogram = calculateColorHistogram(template_image, roi);
        cloverTemplateHistograms[cloverTypes[i]] = histogram;
        
        // 保留原有哈希方法作为备用
        QString hash = calculateImageHash(template_image, roi);
        cloverTemplateHashes[cloverTypes[i]] = hash;
        
        // qDebug() << "成功加载四叶草模板:" << cloverTypes[i] << "颜色直方图特征数:" << histogram.size();
        
        // 保存模板图像和ROI区域用于调试
        QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
        QDir().mkpath(debugDir);
    }
    
    cloverTemplatesLoaded = !cloverTemplateHistograms.isEmpty();
    if (cloverTemplatesLoaded) {
        qDebug() << "四叶草模板加载完成，总数:" << cloverTemplateHistograms.size();
    } else {
        qDebug() << "四叶草模板加载失败，没有成功加载任何模板";
        addLog("四叶草模板加载失败", LogType::Error);
    }
    
    return cloverTemplatesLoaded;
}

bool StarryCard::loadBindStateTemplate()
{
    QString filePath = ":/images/bind_state/clover_bound.png";
    bindStateTemplate = QImage(filePath);
    
    if (bindStateTemplate.isNull()) {
        qDebug() << "无法加载绑定状态模板，路径:" << filePath;
        addLog("无法加载绑定状态模板", LogType::Error);
        return false;
    }
    
    // 计算绑定状态模板的哈希值
    bindStateTemplateHash = calculateImageHash(bindStateTemplate);
    qDebug() << "绑定状态模板加载成功，哈希值:" << bindStateTemplateHash;
    
    // 保存绑定状态模板用于调试
    QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
    QDir().mkpath(debugDir);
    
    return true;
}

QString StarryCard::calculateImageHash(const QImage& image, const QRect& roi)
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

QVector<double> StarryCard::calculateColorHistogram(const QImage& image, const QRect& roi)
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

double StarryCard::calculateHashSimilarity(const QString& hash1, const QString& hash2)
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

double StarryCard::calculateColorHistogramSimilarity(const QImage& image1, const QImage& image2, const QRect& roi)
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

bool StarryCard::isCloverBound(const QImage& cloverImage)
{
    if (bindStateTemplateHash.isEmpty() || cloverImage.isNull()) {
        qDebug() << "绑定状态检查失败: 模板哈希为空或图像无效";
        return false;
    }
    
    // 绑定状态识别ROI区域：(3,38)开始，宽度6，高度7
    QRect bindStateROI(3, 38, 6, 7);
    
    // 确保ROI区域在图像范围内
    if (!cloverImage.rect().contains(bindStateROI)) {
        qDebug() << "四叶草图像尺寸不正确，图像大小:" << cloverImage.size() << "ROI区域:" << bindStateROI;
        addLog("四叶草图像尺寸不正确，无法进行绑定状态识别", LogType::Warning);
        return false;
    }
    
    // 计算当前图像绑定状态区域的哈希值
    QString currentHash = calculateImageHash(cloverImage, bindStateROI);
    bool match = (currentHash == bindStateTemplateHash);
    qDebug() << "绑定状态:" << (match ? "绑定" : "不绑定");
    
    // 保存当前绑定状态区域用于调试
    // static int bindCheckCount = 0;
    // bindCheckCount++;
    // QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
    // QDir().mkpath(debugDir);
    
    // QImage currentBindROI = cloverImage.copy(bindStateROI);
    // QString currentBindPath = QString("%1/current_bind_roi_%2.png").arg(debugDir).arg(bindCheckCount);
    // if (currentBindROI.save(currentBindPath)) {
    //     qDebug() << "当前绑定状态ROI区域已保存:" << currentBindPath;
    // }
    
    // 比较哈希值，匹配度为1表示完全匹配
    return (currentHash == bindStateTemplateHash);
}

QPair<bool, bool> StarryCard::recognizeClover(const QString& cloverType, bool clover_bound, bool clover_unbound)
{
    if (!cloverTemplatesLoaded) {
        addLog("四叶草模板未加载，无法进行识别", LogType::Error);
        return qMakePair(false, false);
    }
    
    if (!pageTemplatesLoaded) {
        addLog("翻页模板未加载，无法进行识别", LogType::Error);
        return qMakePair(false, false);
    }
    
    if (!hwndGame || !IsWindow(hwndGame)) {
        addLog("游戏窗口无效，无法进行四叶草识别", LogType::Error);
        return qMakePair(false, false);
    }
    
    addLog(QString("开始识别四叶草: %1").arg(cloverType), LogType::Info);
    
    // 步骤1: 翻页到顶部
    qDebug() << "开始翻页到顶部";
    
    // 循环点击上翻按钮直到翻到顶部
    int maxPageUpAttempts = 100;
    for (int attempt = 0; attempt < maxPageUpAttempts; ++attempt) {
        
        if (isPageAtTop()) {
            break;
        }

        leftClickDPI(hwndGame, 532, 539);
        sleepByQElapsedTimer(50);
        
        if (attempt == maxPageUpAttempts - 1) {
            qDebug() << "翻页到顶部失败";
        }
    }
    
    // 步骤2: 先识别当前页的10个四叶草
    qDebug() << "开始识别当前页的四叶草";
    
    QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
    if (!screenshot.isNull()) {
        QRect cloverArea(33, 526, 490, 49);
        QImage cloverStrip = screenshot.copy(cloverArea);
        
        if (!cloverStrip.isNull()) {
            // 识别当前页的10个四叶草
            for (int i = 0; i < 10; ++i) {
                int x_offset = i * 49;
                QRect individualCloverRect(x_offset, 0, 49, 49);
                QImage singleClover = cloverStrip.copy(individualCloverRect);
                
                if (singleClover.isNull()) {
                    continue;
                }
                
                int click_x = 33 + x_offset + 24;
                int click_y = 526 + 24;
                
                if (recognizeSingleClover(singleClover, cloverType, click_x, click_y, clover_bound, clover_unbound)) {
                    bool actualBindState = false;
                    checkCloverBindState(singleClover, clover_bound, clover_unbound, actualBindState);
                    return qMakePair(true, actualBindState);
                }
            }
        }
    }
    
    // 步骤3: 逐页向下翻页并只检查第十个位置
    qDebug() << "开始逐页向下翻页识别";
    
    for (int pageIndex = 0; pageIndex < 30; ++pageIndex) {
        qDebug() << "翻页" << (pageIndex + 1) << "次后检查第十个位置";
        
        // 点击下翻按钮
        leftClickDPI(hwndGame, 535, 563);
        sleepByQElapsedTimer(50);
        
        // 只检查第十个位置（翻页后这个位置会更新）
        QImage screenshotAfterPage = captureWindowByHandle(hwndGame,"主页面");
        if (!screenshotAfterPage.isNull()) {
            QRect cloverAreaAfterPage(33, 526, 490, 49);
            QImage cloverStripAfterPage = screenshotAfterPage.copy(cloverAreaAfterPage);
            
            if (!cloverStripAfterPage.isNull()) {
                // 检查第十个位置（索引为9）
                int x_offset = 9 * 49;
                QRect tenthCloverRect(x_offset, 0, 49, 49);
                QImage tenthClover = cloverStripAfterPage.copy(tenthCloverRect);
                
                if (!tenthClover.isNull()) {
                    int click_x = 33 + x_offset + 24;
                    int click_y = 526 + 24;
                    
                    qDebug() << "第" << (pageIndex + 11) << "个四叶草识别";
                    if (recognizeSingleClover(tenthClover, cloverType, click_x, click_y, clover_bound, clover_unbound)) {
                        bool actualBindState = false;
                        checkCloverBindState(tenthClover, clover_bound, clover_unbound, actualBindState);
                        return qMakePair(true, actualBindState);
                    }
                }
            }
        }

        // 检查是否翻页到底部
        if (isPageAtBottom()) {
            qDebug() << "已翻页到底部，终止识别";
            break;
        }
        else{
            qDebug() << QString("第 %1 次翻页后检查完成，继续下一页").arg(pageIndex + 1);
        }
    }
    
    // 识别失败
    qDebug() << QString("四叶草识别失败: %1").arg(cloverType);
    return qMakePair(false, false);
}

bool StarryCard::loadPageTemplates()
{
    // 加载翻页到顶部模板
    QString pageUpPath = ":/images/position/PageUp.png";
    pageUpTemplate = QImage(pageUpPath);
    
    if (pageUpTemplate.isNull()) {
        qDebug() << "无法加载翻页到顶部模板，路径:" << pageUpPath;
        return false;
    }
    
    // 计算翻页到顶部模板的颜色直方图
    pageUpHistogram = calculateColorHistogram(pageUpTemplate);
    
    // 加载翻页到底部模板
    QString pageDownPath = ":/images/position/PageDown.png";
    pageDownTemplate = QImage(pageDownPath);
    
    if (pageDownTemplate.isNull()) {
        qDebug() << "无法加载翻页到底部模板，路径:" << pageDownPath;
        return false;
    }
    
    // 计算翻页到底部模板的颜色直方图
    pageDownHistogram = calculateColorHistogram(pageDownTemplate);
    
    pageTemplatesLoaded = true;
    qDebug() << "翻页模板加载成功";
    
    return true;
}

void StarryCard::loadPositionTemplates()
{
    positionTemplateHashes.clear();
    
    // 使用Qt资源系统加载位置模板
    // 基于resources_position.qrc中的文件列表
    QStringList positionFiles = {
        ":/images/position/(178,96)排行.png",
        ":/images/position/(675,556)合成屋外.png",
        ":/images/position/(94,260)卡片制作.png",
        ":/images/position/(94,326)卡片强化.png"
    };
    
    for (const QString& filePath : positionFiles) {
        // 从资源路径中提取文件名
        QString fileName = QFileInfo(filePath).fileName();
        
        // 只处理以"("开头的文件
        if (!fileName.startsWith("(")) {
            continue;
        }
        
        // 解析文件名获取坐标 - 文件名格式: "(x,y)描述.png"
        static const QRegularExpression regex("\\((\\d+),(\\d+)\\)(.*)\\.");
        QRegularExpressionMatch match = regex.match(fileName);
        
        if (match.hasMatch()) {
            int x = match.captured(1).toInt();
            int y = match.captured(2).toInt();
            QString description = match.captured(3);
            QString key = QString("(%1,%2)%3").arg(x).arg(y).arg(description);
            
            // 从Qt资源系统加载图片文件
            QImage img(filePath);
            if (img.isNull()) {
                qDebug() << "图片加载失败:" << filePath;
                continue;
            }

            if(img.rect().width() != 20 || img.rect().height() != 20)
            {
                qDebug() << "图片大小不正确:" << filePath;
                continue;
            }
            
            // 计算哈希值
            QString hash = calculateImageHash(img); // 不传入区域，计算整个20*20像素的图片的哈希值
            positionTemplateHashes[key] = hash;
            qDebug() << "键:" << key << "哈希值:" << hash;
        }
        else
        {
            qDebug() << "正则表达式匹配失败，文件名:" << fileName;
        }
    }
    
    qDebug() << "位置模板加载完成，总数:" << positionTemplateHashes.size();
}

void StarryCard::loadSynHousePosTemplates()
{
    synHousePosTemplateHashes.clear();

    QStringList positionFiles = {
        ":/images/position/mainCardEmpty.png",
        ":/images/position/subCardEmpty.png",
        ":/images/position/insuranceEmpty.png",
        ":/images/position/enhanceButtonReady.png",
        ":/images/position/enhanceScrollTop.png",
        ":/images/position/enhanceScrollBottom.png",
    };

    for (const QString& filePath : positionFiles) {
        QString fileName = QFileInfo(filePath).fileName();
        QString key = QFileInfo(filePath).baseName();
        QImage img(filePath);
        if (img.isNull()) {
            qDebug() << "图片加载失败:" << filePath;
            continue;
        }

        QString hash = calculateImageHash(img);
        synHousePosTemplateHashes[key] = hash;
    }
    qDebug() << "合成屋模板加载完成，总数:" << synHousePosTemplateHashes.size();
}

QString StarryCard::recognizeCurrentPosition(QImage screenshot)
{
    // 循环遍历所有的位置模板
    for (auto it = positionTemplateHashes.begin(); it != positionTemplateHashes.end(); ++it) {
        QString key = it.key();
        QString templateHash = it.value();
        
        // 解析键中的坐标信息 - 格式: "(x,y)描述"
        static const QRegularExpression regex("\\((\\d+),(\\d+)\\)(.*)");
        QRegularExpressionMatch match = regex.match(key);
        
        if (match.hasMatch()) {
            int x = match.captured(1).toInt();
            int y = match.captured(2).toInt();
            QString description = match.captured(3);

            if(description == "排行")
            {
                continue; // 跳过排行位置
            }
            
            // 从截图中截取对应的区域 (20x20像素)
            QRect region(x, y, 20, 20);
            
            // 检查区域是否在截图范围内
            if (!screenshot.rect().contains(region)) {
                qDebug() << "区域超出游戏窗口范围，跳过:" << key;
                continue;
            }
            
            // 截取指定区域
            QImage regionImage = screenshot.copy(region);
            if (regionImage.isNull()) {
                qDebug() << "截取区域失败:" << key;
                continue;
            }
            
            // 计算该区域的哈希值
            QString currentHash = calculateImageHash(regionImage);
            
            // 与模板哈希值进行比较
            if (currentHash == templateHash) {
                qDebug() << "找到匹配的位置模板:" << key;
                qDebug() << "返回描述:" << description;
                return key; // 返回位置信息
            }
            else
            {
                QString appDir = QCoreApplication::applicationDirPath();
                QString screenshotsDir = appDir + "/screenshots";
                QDir dir(screenshotsDir);
                if(!dir.exists())
                {
                    dir.mkpath(screenshotsDir);
                }
                QString debugDir = screenshotsDir + "/debug_position";
                QDir().mkpath(debugDir);
                if(regionImage.save(QString("%1/%2.png").arg(debugDir).arg(key)))
                {
                    qDebug() << "截图已保存:" << QString("%1/%2.png").arg(debugDir).arg(key);
                }
                else
                {
                    qDebug() << "截图保存失败:" << QString("%1/%2.png").arg(debugDir).arg(key);
                }
                qDebug() << "未找到匹配的位置模板:" << key;
            }
        } else {
            qDebug() << "无法解析位置模板键:" << key;
        }
    }
    
    // 没有找到匹配的位置
    qDebug() << "未找到匹配的位置模板";
    return QString();
}

BOOL StarryCard::checkSynHousePosState(QImage screenshot, const QRect& pos, const QString& templateName)
{
    QImage synHouseImage = screenshot.copy(pos);
    // QString appDir = QCoreApplication::applicationDirPath();
    // QString screenshotsDir = appDir + "/screenshots";
    // synHouseImage.save(QString("%1/%2.png").arg(screenshotsDir).arg(templateName));
    QString hash = calculateImageHash(synHouseImage);
    return hash == synHousePosTemplateHashes[templateName];
}

BOOL StarryCard::checkSpicePosState(QImage screenshot, const QRect& pos, const QString& templateName)
{
    QImage spiceImage = screenshot.copy(pos);
    return calculateImageHash(spiceImage, spiceTemplateRoi) == spiceTemplateHashes[templateName];
}

bool StarryCard::isPageAtTop()
{
    if (!pageTemplatesLoaded || !hwndGame || !IsWindow(hwndGame)) {
        return false;
    }
    
    // 截取游戏窗口
    QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
    if (screenshot.isNull()) {
        qDebug() << "截图失败，无法检测翻页状态";
        return false;
    }
    
    // 从坐标(532,539)开始，截取5x5的图像
    QRect captureRect(532, 539, 5, 5);
    QImage pageCheckImage = screenshot.copy(captureRect);
    
    if (pageCheckImage.isNull()) {
        qDebug() << "翻页检测区域截取失败";
        return false;
    }
    
    // 保存检测图像用于调试
    static int topCheckCount = 0;
    topCheckCount++;
    QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
    QDir().mkpath(debugDir);
    
    // QString checkImagePath = QString("%1/page_top_check_%2.png").arg(debugDir).arg(topCheckCount);
    // if (pageCheckImage.save(checkImagePath)) {
    //     qDebug() << "翻页顶部检测图像已保存:" << checkImagePath;
    // }
    
    // 计算颜色直方图相似度
    double similarity = calculateColorHistogramSimilarity(pageCheckImage, pageUpTemplate);
    
    qDebug() << "翻页到顶部检测相似度:" << QString::number(similarity, 'f', 4);
    
    // 设置阈值，相似度大于0.95认为翻到顶部
    bool isTop = (similarity >= 0.95);
    if (isTop) {
        qDebug() << "检测到已翻页到顶部";
    }
    
    return isTop;
}

bool StarryCard::isPageAtBottom()
{
    if (!pageTemplatesLoaded || !hwndGame || !IsWindow(hwndGame)) {
        return false;
    }
    
    // 截取游戏窗口
    QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
    if (screenshot.isNull()) {
        qDebug() << "截图失败，无法检测翻页状态";
        return false;
    }
    
    // 从坐标(532,560)开始，截取5x5的图像
    QRect captureRect(532, 560, 5, 5);
    QImage pageCheckImage = screenshot.copy(captureRect);
    
    if (pageCheckImage.isNull()) {
        qDebug() << "翻页检测区域截取失败";
        return false;
    }
    
    // 保存检测图像用于调试
    static int bottomCheckCount = 0;
    bottomCheckCount++;
    QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
    QDir().mkpath(debugDir);
    
    // QString checkImagePath = QString("%1/page_bottom_check_%2.png").arg(debugDir).arg(bottomCheckCount);
    // if (pageCheckImage.save(checkImagePath)) {
    //     qDebug() << "翻页底部检测图像已保存:" << checkImagePath;
    // }
    
    // 计算颜色直方图相似度
    double similarity = calculateColorHistogramSimilarity(pageCheckImage, pageDownTemplate);
    
    qDebug() << "翻页到底部检测相似度:" << QString::number(similarity, 'f', 4);
    
    // 设置阈值，相似度大于0.95认为翻到底部
    bool isBottom = (similarity >= 0.95);
    if (isBottom) {
        qDebug() << "检测到已翻页到底部";
    }
    
    return isBottom;
}

bool StarryCard::checkCloverBindState(const QImage& cloverImage, bool clover_bound, bool clover_unbound, bool& actualBindState)
{
    actualBindState = false;
    
    // 如果不需要检查绑定状态，直接返回true
    if (!clover_bound && !clover_unbound) {
        return true;
    }
    
    // 检查实际绑定状态
    actualBindState = isCloverBound(cloverImage);
    addLog(QString("四叶草绑定状态: %1").arg(actualBindState ? "绑定" : "未绑定"), LogType::Info);
    
    // 检查绑定状态是否符合要求
    bool bindStateMatches = false;
    if (clover_bound && clover_unbound) {
        // 两种状态都接受
        bindStateMatches = true;
    } else if (clover_bound && !clover_unbound) {
        // 只接受绑定状态
        bindStateMatches = actualBindState;
    } else if (!clover_bound && clover_unbound) {
        // 只接受未绑定状态
        bindStateMatches = !actualBindState;
    }
    
    if (!bindStateMatches) {
        addLog("四叶草绑定状态不符合要求，继续寻找", LogType::Info);
    }
    
    return bindStateMatches;
}

bool StarryCard::recognizeSingleClover(const QImage& cloverImage, const QString& cloverType, int positionX, int positionY, 
                                         bool clover_bound, bool clover_unbound)
{
    if (cloverImage.isNull()) {
        return false;
    }
    
    // 使用颜色直方图进行匹配
    QRect cloverROI(4, 4, 38, 24);
    double similarity = 0.0;
    if (cloverTemplateImages.contains(cloverType)) {
        QImage templateImage = cloverTemplateImages[cloverType];
        similarity = calculateColorHistogramSimilarity(cloverImage, templateImage, cloverROI);
    }
    
    // 输出相似度用于调试
    qDebug() << "四叶草与" << cloverType << "的相似度:" << QString::number(similarity, 'f', 4);
    
    // 设置相似度阈值
    double similarityThreshold = 1.00; // 100%相似度
    
    // 检查是否匹配
    if (similarity >= similarityThreshold) {
        addLog(QString("找到匹配的四叶草: %1").arg(cloverType), LogType::Success);
        
        // 检查绑定状态
        bool actualBindState = false;
        if (checkCloverBindState(cloverImage, clover_bound, clover_unbound, actualBindState)) {
            // 点击四叶草中心位置
            leftClickDPI(hwndGame, positionX, positionY);
            addLog(QString("点击四叶草中心位置: (%1, %2)").arg(positionX).arg(positionY), LogType::Success);
            return true;
        }
    }
    
    return false;
}

// ================== 香料识别功能实现 ==================

bool StarryCard::loadSpiceTemplates()
{
    // 注意：这里包含"永久保鲜袋"，与UI显示的9种香料不完全一致
    QStringList spiceTypes = {
        "上等香料", "天然香料", "天使香料", "圣灵香料", "极品香料",
        "秘制香料", "精灵香料", "魔幻香料", "皇室香料", "永久保鲜袋"
    };
    
    spiceTemplateHashes.clear();
    
    for (const QString& spiceType : spiceTypes) {
        QString filePath = QString(":/images/spices/%1.png").arg(spiceType);
        QImage template_image(filePath);
        
        if (template_image.isNull()) {
            qDebug() << "无法加载香料模板:" << spiceType << "路径:" << filePath;
            continue;
        }
        
        // 计算并保存模板哈希值
        spiceTemplateHashes.insert(spiceType, calculateImageHash(template_image, spiceTemplateRoi));
    }
    
    spiceTemplatesLoaded = !spiceTemplateHashes.isEmpty();
    if (spiceTemplatesLoaded) {
        qDebug() << "香料模板加载完成，总数:" << spiceTemplateHashes.size();
    } else {
        qDebug() << "香料模板加载失败，没有成功加载任何模板";
    }
    
    return spiceTemplatesLoaded;
}

QPair<bool, bool> StarryCard::recognizeSpice(const QString& spiceType, bool spice_bound, bool spice_unbound)
{
    if (!spiceTemplatesLoaded) {
        qDebug() << "香料模板未加载，无法进行识别";
        return qMakePair(false, false);
    }
    
    if (!hwndGame || !IsWindow(hwndGame)) {
        qDebug() << "游戏窗口无效，无法进行香料识别";
        return qMakePair(false, false);
    }
    
    qDebug() << QString("开始识别香料: %1").arg(spiceType);
    
    // 循环点击上翻按钮直到翻到顶部
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (isPageAtTop()) {
            qDebug() << QString("成功翻页到顶部，总共点击 %1 次").arg(attempt);
            break;
        }

        leftClickDPI(hwndGame, 532, 539);
        sleepByQElapsedTimer(100);
        
        if (attempt == 19) {
            qDebug() << "翻页到顶部失败，已达最大尝试次数";
        }
    }
    
    // 步骤2: 识别当前位置的10个香料
    qDebug() << "开始识别当前位置的香料";
    
        QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
        if (screenshot.isNull()) {
        qDebug() << "截取游戏窗口失败";
        return qMakePair(false, false);
        }
        
        // 以坐标(33,526)为左上角，截取宽度为490像素，高度为49像素的区域
        QRect spiceArea(33, 526, 490, 49);
        QImage spiceStrip = screenshot.copy(spiceArea);
        
    for (int pageIndex = 0; pageIndex < 30; ++pageIndex) {
        if (!spiceStrip.isNull()) {
            // 将此区域在宽度上分成十张，每张图片49*49像素
            for (int i = 0; i < 10; ++i) {
                int x_offset = i * 49;
                QRect individualSpiceRect(x_offset, 0, 49, 49);
                QImage singleSpice = spiceStrip.copy(individualSpiceRect);
                
                if (singleSpice.isNull()) {
                    continue;
                }
                
                int click_x = 33 + x_offset + 24;  // 计算香料中心位置
                int click_y = 526 + 24;
                
                if (recognizeSingleSpice(singleSpice, spiceType, click_x, click_y, spice_bound, spice_unbound)) {
                    return qMakePair(true, isSpiceBound(singleSpice));
                }
            }
        }
    }

    // 步骤3: 逐页向下翻页并只检查第十个位置
    qDebug() << "开始逐页向下翻页识别";
    
    for (int pageIndex = 0; pageIndex < 30; ++pageIndex) {
        qDebug() << QString("翻页 %1 次后检查第十个位置").arg(pageIndex + 1);
        
        // 点击下翻按钮
        leftClickDPI(hwndGame, 535, 563);
        sleepByQElapsedTimer(200);
        sleepByQElapsedTimer(300);
        
        // 只检查第十个位置（翻页后这个位置会更新）
        QImage screenshotAfterPage = captureWindowByHandle(hwndGame,"主页面");
        if (!screenshotAfterPage.isNull()) {
            QRect spiceAreaAfterPage(33, 526, 490, 49);
            QImage spiceStripAfterPage = screenshotAfterPage.copy(spiceAreaAfterPage);
            
            if (!spiceStripAfterPage.isNull()) {
                // 检查第十个位置（索引为9）
                int x_offset = 9 * 49;
                QRect tenthSpiceRect(x_offset, 0, 49, 49);
                QImage tenthSpice = spiceStripAfterPage.copy(tenthSpiceRect);
                
                if (!tenthSpice.isNull()) {
                    int click_x = 33 + x_offset + 24;
                    int click_y = 526 + 24;
                    
                    qDebug() << QString("第 %1 个香料识别").arg(pageIndex + 11);
                    if (recognizeSingleSpice(tenthSpice, spiceType, click_x, click_y, spice_bound, spice_unbound)) {
                        return qMakePair(true, isSpiceBound(tenthSpice));
                    }
                }
            }
        }
        
        // 检查是否翻页到底部
        if (isPageAtBottom()) {
            qDebug() << "已翻页到底部，终止识别";
            break;
        }
    }
    
    // 识别失败
    qDebug() << QString("香料识别失败: %1").arg(spiceType);
    return qMakePair(false, false);
}

bool StarryCard::recognizeSingleSpice(const QImage& spiceImage, const QString& spiceType, int positionX, int positionY, 
                                      bool spice_bound, bool spice_unbound)
{
    // 检查是否匹配
    if (spiceTemplateHashes[spiceType] == calculateImageHash(spiceImage, spiceTemplateRoi)) {
        qDebug() << QString("找到匹配的香料: %1").arg(spiceType);
        
        // 步骤3: 检查绑定状态
        bool actualBindState = false;
        if (checkSpiceBindState(spiceImage, spice_bound, spice_unbound, actualBindState)) {
            // 步骤5: 点击该香料中心位置
            leftClickDPI(hwndGame, positionX, positionY);
            qDebug() << QString("点击香料中心位置: (%1, %2)").arg(positionX).arg(positionY);
            return true;
        }
    }
    
    return false;
}

bool StarryCard::checkSpiceBindState(const QImage& spiceImage, bool spice_bound, bool spice_unbound, bool& actualBindState)
{
    actualBindState = false;
    
    // 如果不需要检查绑定状态，直接返回true
    if (!spice_bound && !spice_unbound) {
        return true;
    }
    
    // 检查实际绑定状态
    actualBindState = isSpiceBound(spiceImage);
    qDebug() << QString("香料绑定状态: %1").arg(actualBindState ? "绑定" : "未绑定");
    
    // 检查绑定状态是否符合要求
    bool bindStateMatches = false;
    if (spice_bound && spice_unbound) {
        // 两种状态都接受
        bindStateMatches = true;
    } else if (spice_bound && !spice_unbound) {
        // 只接受绑定状态
        bindStateMatches = actualBindState;
    } else if (!spice_bound && spice_unbound) {
        // 只接受未绑定状态
        bindStateMatches = !actualBindState;
    }
    
    if (!bindStateMatches) {
        qDebug() << "香料绑定状态不符合要求，继续寻找";
    }
    
    return bindStateMatches;
}

bool StarryCard::isSpiceBound(const QImage& spiceImage)
{
    if (bindStateTemplateHash.isEmpty() || spiceImage.isNull()) {
        qDebug() << "绑定状态检查失败: 模板哈希为空或图像无效";
        return false;
    }
    
    // 绑定状态识别ROI区域：香料图像的(3,38)开始，宽度为6，高度为7的区域
    QRect bindStateROI(3, 38, 6, 7);
    
    // 确保ROI区域在图像范围内
    if (!spiceImage.rect().contains(bindStateROI)) {
        qDebug() << "香料图像尺寸不正确，图像大小:" << spiceImage.size() << "ROI区域:" << bindStateROI;
        return false;
    }
    
    // 计算当前图像绑定状态区域的哈希值
    QString currentHash = calculateImageHash(spiceImage, bindStateROI);
    bool match = (currentHash == bindStateTemplateHash);
    qDebug() << "香料绑定状态:" << (match ? "绑定" : "不绑定");
    
    // 如果该区域与绑定模板匹配度为1，返回真，否则返回假
    return (currentHash == bindStateTemplateHash);
}

// ================== 配方识别功能实现 ==================

// calculateRecipeHistogram 函数已迁移到 RecipeRecognizer 类

QStringList StarryCard::getAvailableRecipeTypes() const
{
    if (!recipeRecognizer) {
        return QStringList();
    }
    
    // 使用 RecipeRecognizer 获取可用配方类型
    return recipeRecognizer->getAvailableRecipeTypes();
}

void StarryCard::updateRecipeCombo()
{
    if (!recipeCombo) {
        return;
    }
    
    QStringList availableRecipes = getAvailableRecipeTypes();
    if (availableRecipes.isEmpty()) {
        recipeCombo->clear();
        recipeCombo->addItem("无可用配方");
        recipeCombo->setEnabled(false);
        return;
    }
    
    // 保存当前选择
    QString currentSelection = recipeCombo->currentText();
    
    // 更新下拉框内容
    recipeCombo->clear();
    recipeCombo->addItems(availableRecipes);
    recipeCombo->setEnabled(true);
    
    // 尝试恢复之前的选择，如果不存在则显示错误状态
    int index = recipeCombo->findText(currentSelection);
    if (index >= 0) {
        recipeCombo->setCurrentIndex(index);
    } else {
        // 如果之前的选择不存在，不自动选择第一个，而是显示错误状态
        if (!currentSelection.isEmpty() && currentSelection != "无可用配方") {
            addLog(QString("之前选择的配方类型 '%1' 不再可用，请重新选择").arg(currentSelection), LogType::Warning);
        }
        recipeCombo->setCurrentIndex(-1); // 不选择任何项
    }
    
    qDebug() << QString("配方选择下拉框已更新，可用配方: %1").arg(availableRecipes.join(", "));
}

// 发送鼠标消息,计算DPI缩放
BOOL StarryCard::leftClickDPI(HWND hwnd, int x, int y)
{
    double scaleFactor = static_cast<double>(DPI) / 96.0;
    
    // 计算DPI缩放后的坐标
    int scaledX = static_cast<int>(x * scaleFactor);
    int scaledY = static_cast<int>(y * scaleFactor);

    // 发送鼠标消息
    BOOL bResult = PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(scaledX, scaledY));
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(scaledX, scaledY));
    return bResult;
}

// 发送鼠标消息,不计算缩放
BOOL StarryCard::leftClick(HWND hwnd, int x, int y)
{
    // 发送鼠标消息
    BOOL bResult = PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    return bResult;
}

void StarryCard::sleepByQElapsedTimer(int ms)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms)
    {
        QCoreApplication::processEvents(); // 不停地处理事件，让程序保持响应
    }
}

// 检测颜色是否符合指定游戏平台的特征颜色
BOOL StarryCard::isGamePlatformColor(COLORREF color, int platformType)
{
    color &= 0x00ffffff;  // 确保只保留RGB分量
    
    BYTE r = bgrRValue(color);
    BYTE g = bgrGValue(color);
    BYTE b = bgrBValue(color);
    
    switch (platformType)
    {
    case 1: // 4399平台 - 浅黄色背景
        return r >= 246 && r <= 255 &&
               g >= 240 && g <= 252 &&
               b >= 214 && b <= 226;
               
    case 2: // QQ空间
        // 目标颜色 0xfdffea (R=253, G=255, B=234)
        return (r == 253) && (g == 255) && (b == 234);
        
    case 3: // QQ大厅 - 深灰色
        // 目标颜色 0x3c3c3d (R=60, G=60, B=61)
        return (r == 60) && (g == 60) && (b == 61);
        
    case 4: // 纯白色
        return (r == 255) && (g == 255) && (b == 255);
        
    case 5: // 断网界面 - 浅蓝灰色
        return r >= 211 && r <= 213 &&
               g >= 227 && g <= 229 &&
               b >= 236 && b <= 237;
    }
    return FALSE;
}

BOOL StarryCard::getWindowBitmap(HWND hwnd, int& width, int& height, COLORREF*& pixelData)
{
    // 初始化输出参数
    width = 0;
    height = 0;
    pixelData = nullptr;
    
    // 验证窗口句柄
    if (!hwnd || !IsWindow(hwnd)) {
        addLog("无效的窗口句柄", LogType::Error);
        return FALSE;
    }
    
    // 释放之前的全局像素数据
    if (globalPixelData != nullptr) {
        delete[] globalPixelData;
        globalPixelData = nullptr;
        globalBitmapWidth = 0;
        globalBitmapHeight = 0;
    }
    
    // 获取窗口整体尺寸（包含标题栏等）
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        addLog("获取窗口矩形失败", LogType::Error);
        return FALSE;
    }
    
    width = windowRect.right - windowRect.left;
    height = windowRect.bottom - windowRect.top;
    
    if (width <= 0 || height <= 0) {
        addLog("窗口尺寸无效", LogType::Error);
        return FALSE;
    }
    
    // 获取窗口设备上下文
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) {
        addLog("获取窗口设备上下文失败", LogType::Error);
        return FALSE;
    }
    
    // 创建兼容设备上下文
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    if (!hdcMem) {
        addLog("创建兼容设备上下文失败", LogType::Error);
        ReleaseDC(hwnd, hdcWindow);
        return FALSE;
    }
    
    // 创建兼容位图
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    if (!hBitmap) {
        addLog("创建兼容位图失败", LogType::Error);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return FALSE;
    }
    
    // 选择位图到内存设备上下文
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    
    // 复制窗口内容到位图
    if (!BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY)) {
        addLog("复制窗口内容到位图失败", LogType::Error);
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return FALSE;
    }
    
    // 分配全局像素数据内存
    int pixelCount = width * height;
    globalPixelData = new(std::nothrow) COLORREF[pixelCount];
    if (!globalPixelData) {
        qDebug() << "分配像素数据内存失败";
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return FALSE;
    }
    
    // 设置BITMAPINFO结构
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;  // 负值表示自上而下的位图
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;     // 32位色深
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // 获取位图像素数据
    int scanLines = GetDIBits(hdcMem, hBitmap, 0, height, globalPixelData, &bmi, DIB_RGB_COLORS);
    if (scanLines == 0) {
        qDebug() << "获取位图像素数据失败";
        delete[] globalPixelData;
        globalPixelData = nullptr;
        width = 0;
        height = 0;
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWindow);
        return FALSE;
    }
    else
        qDebug() << QString("获取位图像素数据成功,行数为%1").arg(scanLines);
    
    // 设置全局变量和输出参数
    globalBitmapWidth = width;
    globalBitmapHeight = height;
    pixelData = globalPixelData;
    
    // 清理资源
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);
    
    addLog(QString("成功获取窗口位图：%1x%2").arg(width).arg(height), LogType::Success);
    return TRUE;
}

// 在大厅窗口中等待最近服务器出现，返回所属平台
int StarryCard::waitServerInWindow(RECT rectServer, int *px, int *py)
{
    // 首先检查大厅窗口是否有效
    if (!hwndHall || !IsWindow(hwndHall))
    {
        addLog("大厅窗口句柄无效，无法进行服务器识别", LogType::Error);
        return -1;
    }

    addLog("开始等待选服窗口出现...", LogType::Info);

    int times = 0;
    while (times < 10) // 改为明确的条件，而不是while(true)
    {
        times++;
        addLog(QString("第%1次尝试识别服务器窗口...").arg(times), LogType::Info);

        // 首先检查是否有游戏窗口（微端情况）
        if (getActiveGameWindow(hwndHall))
        {
            addLog("发现游戏窗口，识别为微端，无需选服", LogType::Success);
            return 0;
        }

        // 依次查找4399、QQ空间、QQ大厅，纯白，断网色块，找到直接返回
        for (int platformType = 1; platformType <= 4; platformType++)
        {
            int result = findLatestServer(platformType, px, py);
            if (result == platformType)
            {
                qDebug() << QString("找到选服窗口：平台类型%1").arg(platformType);
                return platformType;
            }
            // 如果findLatestServer返回-1，说明位图获取失败，提前退出
            if (result == -1)
            {
                qDebug() << "位图获取失败，停止识别";
                return -1;
            }
        }
        int platformType = findLatestServer(5, px, py);
        if (platformType == 5)
        {
            int retryX = (rectServer.right - rectServer.left) / 2;
            int retryY = 350 * DPI / 96;
            leftClick(hwndServer, retryX, retryY);
            qDebug() << "找到断网窗口，点击重试按钮位置:" << retryX << "," << retryY;
            sleepByQElapsedTimer(2000); // 等待2秒
            hwndServer = getActiveServerWindow(hwndHall);
        }

        addLog(QString("第%1次识别未找到匹配的平台，等待1秒后重试...").arg(times), LogType::Warning);

        // 使用Qt事件循环进行安全延时，保持界面响应
        sleepByQElapsedTimer(1000);
    }

    addLog("等待选服窗口超时，所有尝试均失败", LogType::Error);
    return -1;
}

int StarryCard::findLatestServer(int platformType, int *px, int *py)
{
    // 验证输入参数
    if (!px || !py) {
        addLog("findLatestServer参数无效", LogType::Error);
        return -1;
    }
    
    // 验证大厅窗口
    if (!IsWindowVisible(hwndHall)) {
        addLog("大厅窗口异常，无法进行服务器识别", LogType::Error);
        return -1;
    }
    
    int hallWidth = 0, hallHeight = 0; // 大厅窗口尺寸
    if (!getWindowBitmap(hwndHall, hallWidth, hallHeight, globalPixelData)) {
        addLog("获取大厅窗口位图失败", LogType::Error);
        return -1; // 获取大厅窗口失败，返回-1表示错误
    }

    // 构造二级指针
    COLORREF *pHallShot[4320]; //大厅截图二级指针，高度不超过8K分辨率4320
    for (int i = 0; i < hallHeight; i++) {
        pHallShot[i] = globalPixelData + i * hallWidth;
    }

    // 根据平台类型设置识别区域尺寸
    int width, height;
    switch (platformType) {
        case 1: // 4399平台
            width = 220; height = 135;
            break;
        case 2: // QQ空间
            width = 210; height = 70;
            break;
        case 3: // QQ大厅
        case 4: // 纯白色检验（QQ空间登录界面）
        case 5: // 断网界面
            width = 220; height = 50;
            break;
        default:
            addLog(QString("不支持的平台类型：%1").arg(platformType), LogType::Error);
            return 0;
    }

    int step = 2;   // 识图步长
    
    // 从右到左，从上到下扫描
    int processCount = 0; // 计数器，用于定期处理事件
    for (int x = hallWidth - width; x >= 0; x -= step) {
        for (int y = 0; y <= hallHeight - height; y += step) {
            // 创建当前扫描区域
            QRect scanRegion(x, y, width, height);
            
            // 进行颜色识别
            int result = recognizeBitmapRegionColor(platformType, pHallShot, scanRegion);
            if (result == platformType) {
                *px = x;
                *py = y;
                addLog(QString("找到平台类型%1的色块：位置(%2,%3)，尺寸(%4x%5)")
                       .arg(platformType).arg(x).arg(y).arg(width).arg(height), LogType::Success);
                return platformType;
            }
            
            // 每处理100次循环就处理一次事件，保持UI响应
            processCount++;
            if (processCount % 100 == 0) {
                QCoreApplication::processEvents();
                // 可选：检查是否需要中断扫描（比如用户取消操作）
            }
        }
    }

    addLog(QString("未找到平台类型%1的色块").arg(platformType), LogType::Warning);
    return 0; // 未找到
}

int StarryCard::recognizeBitmapRegionColor(int platformType, COLORREF *pHallShot[4320], const QRect& region)
{
    int step = 2;
    
    // 遍历指定矩形区域内的所有像素，使用绝对坐标
    for (int x = region.width() - 1; x >= 0; x -= step)
    {
        for (int y = 0; y < region.height(); y += step)
        {
            // 检测颜色是否匹配当前平台类型
            if (!isGamePlatformColor(pHallShot[region.y() + y][region.x() + x], platformType))
            {
                return 0; // 返回0表示未识别成功
            }
        }
    }

    return platformType;  // 返回识别成功
}

//找到一个小号子窗口下唯一的选服窗口
HWND StarryCard::getServerWindow(HWND hWndChild)
{
    // 验证输入参数
    if (!hWndChild || !IsWindow(hWndChild)) {
        return nullptr;
    }
    
    // 查找一级子窗口
    HWND hWnd1 = FindWindowEx(hWndChild, nullptr, L"CefBrowserWindow", nullptr);
    if (hWnd1 == nullptr || !IsWindow(hWnd1)) {
        return nullptr;
    }
    
    // 查找二级子窗口
    HWND hWnd2 = FindWindowEx(hWnd1, nullptr, L"Chrome_WidgetWin_0", nullptr);
    if (hWnd2 == nullptr || !IsWindow(hWnd2)) {
        return nullptr;
    }
    
    // 查找三级子窗口（选服窗口）
    HWND hWndServer = FindWindowEx(hWnd2, nullptr, L"Chrome_RenderWidgetHostHWND", nullptr);
    if (hWndServer && IsWindow(hWndServer)) {
        return hWndServer;
    }
    
    return nullptr;
}

//找到游戏大厅当前显示的选服窗口，找不到返回nullptr
HWND StarryCard::getActiveServerWindow(HWND hWndHall)
{
    // 验证输入参数
    if (!hWndHall || !IsWindow(hWndHall)) {
        addLog("传入的大厅窗口句柄无效", LogType::Error);
        return nullptr;
    }
    
    wchar_t className[256];
    if (!GetClassNameW(hWndHall, className, 256)) {
        addLog("无法获取窗口类名", LogType::Error);
        return nullptr;
    }
    
    // 如果是微端窗口，直接返回
    if (wcscmp(className, L"ApolloRuntimeContentWindow") == 0) {
        addLog("找到微端窗口", LogType::Success);
        return hWndHall; //微端的选服窗口就是微端窗口
    }
    
    // 不是微端也不是大厅窗口
    if (wcscmp(className, L"DUIWindow") != 0) {
        addLog(QString("不支持的窗口类型：%1").arg(QString::fromWCharArray(className)), LogType::Warning);
        return nullptr;
    }
    
    // 判断为游戏大厅窗口
    addLog("找到大厅窗口，窗口名：" + QString::fromWCharArray(className), LogType::Success);
    
    HWND hWndChild = nullptr, hWndServer = nullptr;
    int searchCount = 0;
    const int maxSearchCount = 10; // 最多搜索10个Tab窗口
    
    while (searchCount < maxSearchCount) {
        hWndChild = FindWindowEx(hWndHall, hWndChild, L"TabContentWnd", nullptr); // 查找一个小号窗口
        if (hWndChild == nullptr) {
            addLog(QString("搜索了%1个Tab窗口，未找到有效的选服窗口").arg(searchCount), LogType::Info);
            return nullptr; // 没有小号窗口了，还没找到活跃窗口，就返回nullptr
        }
        
        searchCount++;
        addLog(QString("正在检查第%1个Tab窗口...").arg(searchCount), LogType::Info);
        
        hWndServer = getServerWindow(hWndChild);
        if (hWndServer != nullptr) {
            addLog(QString("在第%1个Tab窗口中找到选服窗口").arg(searchCount), LogType::Success);
            return hWndServer;
        }
    }
    
    addLog(QString("已搜索%1个Tab窗口，未找到有效的选服窗口").arg(maxSearchCount), LogType::Warning);
    return nullptr;
}

BOOL StarryCard::refreshGameWindow()
{
    HWND hwndOrigin = getActiveGameWindow(hwndHall);
    qDebug() << QString("刷新前游戏窗口：%1").arg(QString::number(reinterpret_cast<quintptr>(hwndOrigin), 10));

    // 点击刷新按钮
    clickRefresh();

    int counter = 0;
    while(hwndOrigin && hwndOrigin == getActiveGameWindow(hwndHall))
    {
        counter++;
        if(counter > 50) // 5秒
        {
            qDebug() << "刷新失败，点击刷新按钮无效";
            return FALSE;
        }

        sleepByQElapsedTimer(200);
    }
    qDebug() << "刷新成功";

    sleepByQElapsedTimer(1000);

    hwndServer = getActiveServerWindow(hwndHall);
    if (!hwndServer) {
        qDebug() << "未找到选服窗口，可能是微端或窗口已关闭";
        // 继续执行后续识别，不需要矩形信息
    } else {
        qDebug() << QString("找到选服窗口：%1").arg(QString::number(reinterpret_cast<quintptr>(hwndServer), 10));
        
        // 验证选服窗口是否有效
        if (!IsWindow(hwndServer)) {
            qDebug() << "选服窗口句柄无效";
            hwndServer = nullptr;
        }
    }

    int serverWidth = 0, serverHeight = 0;
    RECT rectHall = {0};
    RECT rectServer = {0};
    
    // 安全地获取大厅窗口矩形
    if (!GetWindowRect(hwndHall, &rectHall)) {
        qDebug() << "获取大厅窗口矩形失败";
    }
    
    // 只有当选服窗口有效时才获取其矩形
    if (hwndServer && IsWindow(hwndServer)) {
        if (GetWindowRect(hwndServer, &rectServer)) {
            serverWidth = rectServer.right - rectServer.left;
            serverHeight = rectServer.bottom - rectServer.top;
            qDebug() << QString("选服窗口尺寸：%1x%2").arg(serverWidth).arg(serverHeight);
        } else {
            qDebug() << "获取选服窗口矩形失败";
        }
    }

    // 添加截图功能：分别截取大厅窗口和选服窗口的图片
    addLog("开始截取窗口图片...", LogType::Info);
    
    // 确保screenshots文件夹存在
    QString appDir = QCoreApplication::applicationDirPath();
    QString screenshotsDir = appDir + "/screenshots";
    QDir dir(screenshotsDir);
    if (!dir.exists()) {
        if (!dir.mkpath(screenshotsDir)) {
            qDebug() << "创建screenshots文件夹失败";
        } else {
            qDebug() << "创建screenshots文件夹成功";
        }
    }
    
    // 截取大厅窗口
    if (hwndHall && IsWindow(hwndHall)) {
        QImage hallImage = captureWindowByHandle(hwndHall, "大厅");
        if (!hallImage.isNull()) {
            QString hallFilename = QString("%1/Ahall_window.png").arg(screenshotsDir);
            if (hallImage.save(hallFilename)) {
                qDebug() << "大厅窗口截图保存成功";
            } else {
                qDebug() << "大厅窗口截图保存失败";
            }
        }
    }
    
    // 截取选服窗口
    if (hwndServer && IsWindow(hwndServer)) {
        QImage serverImage = captureWindowByHandle(hwndServer, "选服");
        if (!serverImage.isNull()) {
            QString serverFilename = QString("%1/Aserver_window.png").arg(screenshotsDir);
            if (serverImage.save(serverFilename)) {
                qDebug() << "选服窗口截图保存成功";
            } else {
                qDebug() << "选服窗口截图保存失败";
            }
        }
    } else {
        qDebug() << "选服窗口无效，跳过截图";
    }

    int x = 0;
    int y = 0;
    int platformType = waitServerInWindow(rectServer, &x, &y);

    //根据平台和大厅判定坐标确定最近服务器在选服窗口中的坐标
    int latestServerX = 0, latestServerY = 0;//最近服务器在选服窗口中的坐标

    if (platformType == -1)
    {
        qDebug() << "无法识别服务器窗口";
        return FALSE;
    }
    else if (platformType == 0)
    {
        qDebug() << "找到微端窗口，无需选服";
    }
    else if (platformType == 1)
    {
        latestServerX = (x - 285) * DPI / 96 + rectHall.left - rectServer.left;
        latestServerY = (y + 45) * DPI / 96 + rectHall.top - rectServer.top;
        qDebug() << QString("找到4399的选服窗口，位置(%2,%3)").arg(latestServerX).arg(latestServerY);
    }
    else if (platformType == 2)
    {
        latestServerX = (rectServer.right - rectServer.left) / 2 - 115 * DPI / 96; // 中央位置左偏115
        latestServerY = 267 * DPI / 96;
        qDebug() << QString("找到QQ空间的选服窗口，位置(%2,%3)").arg(x).arg(y);
    }
    else if (platformType == 3)
    {
        latestServerX = (rectServer.right - rectServer.left) / 2 + 30 * DPI / 96; // 中央位置右偏30
        latestServerY = 580 * DPI / 96;
        qDebug() << QString("找到QQ大厅的选服窗口，位置(%2,%3)").arg(x).arg(y);
    }
    else if (platformType == 4)
    {
        qDebug() << "找到选服窗口的纯白色色块";
    }
    else
    {
        qDebug() << "未知的平台类型";
        return FALSE;
    }

    hwndGame = nullptr; // 刷新后重新获取游戏窗口
    counter = 0;
    while(hwndGame == nullptr) //每1000ms获取1次游戏窗口
    {
        counter++;
        if (counter > 10)
        {
            qDebug() << "无法进入服务器";
            return FALSE;
        }
        if(platformType >= 1 && platformType <= 3) // 1为4399，2为QQ空间，3为QQ大厅,需要点击最近服务器位置
        {
            leftClick(hwndServer, latestServerX, latestServerY);
            qDebug() << "点击选服窗口，位置:" << latestServerX << "," << latestServerY;
        }
        sleepByQElapsedTimer(1000);
        hwndGame = getActiveGameWindow(hwndHall);
        if (hwndGame)
        {
            updateHandleDisplay(hwndGame);
            qDebug() << "找到游戏窗口:" << hwndGame;
            return TRUE;
        }
    }
    return FALSE;
}

// 计算汉明距离 (差异比特数)
int StarryCard::hammingDistance(uint64_t hash1, uint64_t hash2) {
    uint64_t xorResult = hash1 ^ hash2;
    int distance = 0;
    while (xorResult) {
        distance += xorResult & 1;
        xorResult >>= 1;
    }
    return distance;
}

// 图片相似度匹配接口 (返回汉明距离)
int StarryCard::matchImages(const QString& path, uint64_t hash) {
    QImage imgTemplate(path);
    
    if (imgTemplate.isNull()) {
        qWarning() << "Failed to load images";
        return -1;
    }
    
    uint64_t hashTemplate = calculateImageHash(imgTemplate).toULongLong();

    return hammingDistance(hash, hashTemplate);
}

BOOL StarryCard::closeHealthTip(uint8_t retryCount)
{
    if(!IsWindowVisible(hwndGame))
    {
        qDebug() << "游戏窗口未显示，无法关闭健康提示";
        return FALSE;
    }

    // 健康提示模板
    QString healthTipPath = ":/images/position/healthyTip.png";
    QImage imgTemplate(healthTipPath);
    QString hashHealthyTip = calculateImageHash(imgTemplate);

    // 等待健康提示出现，最多等待10秒
    retryCount = retryCount > 10 ? 10 : retryCount;
    
    for (int i = 0; i < retryCount; i++)
    {
        QImage imgGame = captureWindowByHandle(hwndGame,"主页面");
        if (imgGame.isNull())
        {
            qDebug() << "获取游戏窗口截图失败";
            return FALSE;
        }

        QString hashHealthyTipCurrent = calculateImageHash(imgGame, QRect(378, 330, 20, 20));
        QString hashRankCurrent = calculateImageHash(imgGame, QRect(178, 96, 20, 20));

        if (hashHealthyTipCurrent == hashHealthyTip) // 健康提示出现
        {
            // 点击关闭健康提示
            leftClickDPI(hwndGame, 588, 204);
            qDebug() << "点击关闭健康提示成功";
            sleepByQElapsedTimer(100); // 等待100毫秒
            if(hashRankCurrent != positionTemplateHashes["(178,96)排行"]) // 健康提示出现且排行榜未出现，视为有假期特惠挡住
            {
                // 点击关闭假期特惠
                leftClickDPI(hwndGame, 840, 44);
                qDebug() << "点击关闭假期特惠成功";
            }
            return TRUE;
        }
        sleepByQElapsedTimer(1000);
    }
    qDebug() << "健康提示未显示";
    return FALSE;
}

// BOOL StarryCard::goToPageCardEnhance(uint8_t retryCount)
// {
//     for(int i = 0; i < retryCount; i++)
//     {
//         QImage screenshot = captureWindowByHandle(hwndGame);
//         QString position = recognizeCurrentPosition(screenshot);
//         if(position == "(94,326)卡片强化")
//         {
//             addLog("成功前往卡片强化页面", LogType::Success);
//             return TRUE; // 已经是卡片强化页面，直接返回
//         }
//         else if(position == "(94,260)卡片制作")
//         {
//             leftClickDPI(hwndGame, CARD_ENHANCE_POS.x(), CARD_ENHANCE_POS.y()); //点击卡片强化按钮
//         }
//         else if(position == "(675,556)合成屋外")
//         {
//             leftClickDPI(hwndGame, SYNTHESIS_HOUSE_POS.x(), SYNTHESIS_HOUSE_POS.y()); //点击合成屋按钮
//         }
//         sleepByQElapsedTimer(300);
//     }
//     QMessageBox::warning(this, "提示", "前往卡片强化页面失败");
//     return FALSE;
// }  

// BOOL StarryCard::goToPageCardProduce(uint8_t retryCount)
// {
//     for(int i = 0; i < retryCount; i++)
//     {
//         QImage screenshot = captureWindowByHandle(hwndGame);
//         QString position = recognizeCurrentPosition(screenshot);

//         if(position == "(94,260)卡片制作")
//         {
//             addLog("成功前往卡片制作页面", LogType::Success);
//             return TRUE; // 已经是卡片制作页面，直接返回
//         }
//         else if(position == "(94,326)卡片强化")
//         {
//             leftClickDPI(hwndGame, CARD_PRODUCE_POS.x(), CARD_PRODUCE_POS.y()); //点击卡片制作按钮
//         }
//         else if(position == "(675,556)合成屋外")
//         {
//             leftClickDPI(hwndGame, SYNTHESIS_HOUSE_POS.x(), SYNTHESIS_HOUSE_POS.y()); //点击合成屋按钮
//         }
//         sleepByQElapsedTimer(300);
//     }
//     QMessageBox::warning(this, "提示", "前往卡片制作页面失败");
//     return FALSE;
// }

BOOL StarryCard::goToPage(PageType targetPage, uint8_t retryCount)
{
    QString targetPageName;
    if(targetPage == PageType::CardEnhance)
    {
        targetPageName = "卡片强化";
    }
    else if(targetPage == PageType::CardProduce)
    {
        targetPageName = "卡片制作";
    }

    for(int i = 0; i < retryCount; i++)
    {
        QImage screenshot = captureWindowByHandle(hwndGame,"主页面");
        QString position = recognizeCurrentPosition(screenshot);

        if(position.contains(targetPageName))
        {
            qDebug() << QString("成功前往%1页面").arg(targetPageName);
            return TRUE;
        }
        else if(position == "(94,260)卡片制作")
        {
            leftClickDPI(hwndGame, CARD_ENHANCE_POS.x(), CARD_ENHANCE_POS.y()); //点击卡片强化按钮
        }
        else if(position == "(94,326)卡片强化")
        {
            leftClickDPI(hwndGame, CARD_PRODUCE_POS.x(), CARD_PRODUCE_POS.y()); //点击卡片制作按钮
        }
        else if(position == "(675,556)合成屋外")
        {
            leftClickDPI(hwndGame, SYNTHESIS_HOUSE_POS.x(), SYNTHESIS_HOUSE_POS.y()); //点击合成屋按钮
        }
        sleepByQElapsedTimer(300);
    }
    qDebug() << QString("前往%1页面失败").arg(targetPageName);
    return FALSE;
}

// ================================== 全局配置加载方法 ==================================

bool StarryCard::loadGlobalEnhancementConfig()
{
    // 清空现有配置
    g_enhancementConfig = GlobalEnhancementConfig();
    
    QFile file(enhancementConfigPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开强化配置文件:" << enhancementConfigPath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        qDebug() << "JSON根节点不是对象";
        return false;
    }
    
    QJsonObject config = doc.object();
    
    // 加载基本等级设置
    if (config.contains("max_enhancement_level") && config["max_enhancement_level"].isDouble()) {
        g_enhancementConfig.maxLevel = config["max_enhancement_level"].toInt();
    }
    
    if (config.contains("min_enhancement_level") && config["min_enhancement_level"].isDouble()) {
        g_enhancementConfig.minLevel = config["min_enhancement_level"].toInt();
    }
    
    // 加载各等级的详细配置
    for (int level = 0; level < 14; ++level) {
        QString levelKey = QString("%1-%2").arg(level).arg(level + 1);
        
        if (!config.contains(levelKey)) continue;
        
        QJsonObject levelConfigJson = config[levelKey].toObject();
        GlobalEnhancementConfig::LevelConfig levelConfig;
        
        // 加载副卡配置
        if (levelConfigJson.contains("subcard1") && levelConfigJson["subcard1"].isDouble()) {
            levelConfig.subcard1 = levelConfigJson["subcard1"].toInt();
        }
        if (levelConfigJson.contains("subcard2") && levelConfigJson["subcard2"].isDouble()) {
            levelConfig.subcard2 = levelConfigJson["subcard2"].toInt();
        }
        if (levelConfigJson.contains("subcard3") && levelConfigJson["subcard3"].isDouble()) {
            levelConfig.subcard3 = levelConfigJson["subcard3"].toInt();
        }
        
        // 加载四叶草配置
        if (levelConfigJson.contains("clover") && levelConfigJson["clover"].isString()) {
            levelConfig.clover = levelConfigJson["clover"].toString();
        }
        if (levelConfigJson.contains("clover_bound") && levelConfigJson["clover_bound"].isBool()) {
            levelConfig.cloverBound = levelConfigJson["clover_bound"].toBool();
        }
        if (levelConfigJson.contains("clover_unbound") && levelConfigJson["clover_unbound"].isBool()) {
            levelConfig.cloverUnbound = levelConfigJson["clover_unbound"].toBool();
        }
        
        // 加载主卡配置
        if (levelConfigJson.contains("main_card_type") && levelConfigJson["main_card_type"].isString()) {
            levelConfig.mainCardType = levelConfigJson["main_card_type"].toString();
        }
        if (levelConfigJson.contains("main_card_bound") && levelConfigJson["main_card_bound"].isBool()) {
            levelConfig.mainCardBound = levelConfigJson["main_card_bound"].toBool();
        }
        if (levelConfigJson.contains("main_card_unbound") && levelConfigJson["main_card_unbound"].isBool()) {
            levelConfig.mainCardUnbound = levelConfigJson["main_card_unbound"].toBool();
        }
        
        // 加载副卡类型配置
        if (levelConfigJson.contains("sub_card_type") && levelConfigJson["sub_card_type"].isString()) {
            levelConfig.subCardType = levelConfigJson["sub_card_type"].toString();
        }
        if (levelConfigJson.contains("sub_card_bound") && levelConfigJson["sub_card_bound"].isBool()) {
            levelConfig.subCardBound = levelConfigJson["sub_card_bound"].toBool();
        }
        if (levelConfigJson.contains("sub_card_unbound") && levelConfigJson["sub_card_unbound"].isBool()) {
            levelConfig.subCardUnbound = levelConfigJson["sub_card_unbound"].toBool();
        }
        
        // 存储到全局配置中
        g_enhancementConfig.setLevelConfig(level, level + 1, levelConfig);
    }
    
    qDebug() << "全局强化配置加载完成:";
    qDebug() << "- 等级范围:" << g_enhancementConfig.minLevel << "-" << g_enhancementConfig.maxLevel;
    qDebug() << "- 已加载" << g_enhancementConfig.levelConfigs.size() << "个等级配置";
    
    return true;
}

bool StarryCard::loadGlobalSpiceConfig()
{
    // 清空现有配置
    g_spiceConfig.clear();
    
    QFile file("spice_config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开香料配置文件: spice_config.json";
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "香料配置JSON解析错误:" << error.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        qDebug() << "香料配置JSON根节点不是对象";
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray spicesArray = root["spices"].toArray();
    
    // 加载每个香料配置，但只加载used=true的项目
    // 香料等级对应关系：1-天然香料, 2-上等香料, 3-秘制香料, 4-极品香料, 5-皇室香料,
    //                 6-魔幻香料, 7-精灵香料, 8-天使香料, 9-圣灵香料
    for (const QJsonValue& value : spicesArray) {
        QJsonObject spiceObj = value.toObject();
        
        // 检查是否启用
        bool used = spiceObj["used"].toBool(true);  // 默认为true
        if (!used) {
            continue;  // 跳过未启用的香料
        }
        
        GlobalSpiceConfig::SpiceItem spiceItem;
        
        // 加载香料基本信息
        if (spiceObj.contains("name") && spiceObj["name"].isString()) {
            spiceItem.name = spiceObj["name"].toString();
        }
        
        spiceItem.used = used;
        
        if (spiceObj.contains("bound") && spiceObj["bound"].isBool()) {
            spiceItem.bound = spiceObj["bound"].toBool();
        }
        
        if (spiceObj.contains("limitType") && spiceObj["limitType"].isString()) {
            spiceItem.limitType = spiceObj["limitType"].toString();
        }
        
        if (spiceObj.contains("limitAmount") && spiceObj["limitAmount"].isDouble()) {
            spiceItem.limitAmount = spiceObj["limitAmount"].toInt();
        }
        
        if (spiceObj.contains("spiceLevel") && spiceObj["spiceLevel"].isDouble()) {
            spiceItem.spiceLevel = spiceObj["spiceLevel"].toInt();
        }
        
        // 只有name不为空的香料才添加到配置中
        if (!spiceItem.name.isEmpty()) {
            g_spiceConfig.spices.append(spiceItem);
        }
    }
    
    qDebug() << "全局香料配置加载完成:";
    qDebug() << "- 已加载" << g_spiceConfig.spices.size() << "个启用的香料配置";
    
    // 按等级显示香料信息
    QVector<GlobalSpiceConfig::SpiceItem> sortedSpices = g_spiceConfig.getUsedSpicesByLevel();
    if (!sortedSpices.isEmpty()) {
        QStringList spiceInfo;
        for (const auto& spice : sortedSpices) {
            spiceInfo.append(QString("%1(等级%2)").arg(spice.name).arg(spice.spiceLevel));
        }
        qDebug() << "- 启用的香料（按等级排序）:" << spiceInfo.join(", ");
    }
    
    return true;
}

// ================================== 香料配置页面功能 ==================================

QWidget* StarryCard::createSpiceConfigPage()
{
    QWidget* page = new QWidget();
    page->setStyleSheet("background-color: transparent;");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建标题
    QLabel* titleLabel = new QLabel("香料配置");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(R"(
        font-size: 16px; 
        font-weight: bold; 
        color: #003D7A;
        background-color: rgba(125, 197, 255, 150);
        border-radius: 8px;
        padding: 8px;
        margin: 5px 0;
    )");
    layout->addWidget(titleLabel);
    
    // 创建表格
    spiceTable = new QTableWidget(9, 5);
    spiceTable->setStyleSheet(R"(
        QTableWidget {
            gridline-color: rgba(102, 204, 255, 120);
            background-color: rgba(255, 255, 255, 160);
            alternate-background-color: rgba(102, 204, 255, 80);
            border: 2px solid rgba(102, 204, 255, 200);
            border-radius: 8px;
        }
        QTableWidget::item {
            padding: 3px;
            border: none;
        }
        QHeaderView::section {
            background-color: rgba(102, 204, 255, 200);
            color: #003D7A;
            padding: 6px;
            border: 1px solid rgba(102, 204, 255, 150);
            font-weight: bold;
            font-size: 14px;
        }
        QComboBox {
            background-color: rgba(255, 255, 255, 220);
            border: 1px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 2px 4px;
            color: #003D7A;
        }
        QComboBox:hover {
            background-color: rgba(102, 204, 255, 100);
        }
        QComboBox::drop-down {
            border: none;
            width: 18px;
            background-color: rgba(102, 204, 255, 150);
            border-radius: 3px;
        }
        QComboBox::down-arrow {
            image: url(:/images/icons/downArrow.svg);
            width: 12px;
            height: 12px;
            margin-right: 1px;
        }
        QCheckBox {
            color: #003D7A;
            font-weight: bold;
        }
        QCheckBox::indicator {
            width: 20px;
            height: 20px;
            border: 2px solid rgba(102, 204, 255, 200);
            border-radius: 4px;
            background-color: rgba(255, 255, 255, 200);
        }
        QCheckBox::indicator:checked {
            background-color: rgba(102, 204, 255, 200);
            image: url(:/images/icons/勾选.svg);
        }
        QSpinBox {
            background-color: rgba(255, 255, 255, 220);
            border: 1px solid rgba(102, 204, 255, 150);
            border-radius: 4px;
            padding: 2px 4px;
            color: #003D7A;
        }
        QSpinBox:disabled {
            background-color: rgba(128, 128, 128, 100);
            color: rgba(128, 128, 128, 200);
        }
        QSpinBox::up-button {
            image: url(:/images/icons/uparrow_1f.svg);
            background-color: rgba(102, 204, 255, 150);
            border: 1px solid rgba(102, 204, 255, 200);
            border-radius: 3px;
            width: 20px;
            height: 16px;
            subcontrol-origin: border;
            subcontrol-position: top right;
            margin-right: 2px;
            margin-top: 1px;
        }
        QSpinBox::up-button:hover {
            background-color: rgba(102, 204, 255, 200);
        }
        QSpinBox::down-button {
            image: url(:/images/icons/downarrow_1f.svg);
            background-color: rgba(102, 204, 255, 150);
            border: 1px solid rgba(102, 204, 255, 200);
            border-radius: 3px;
            width: 20px;
            height: 16px;
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            margin-right: 2px;
            margin-bottom: 1px;
        }
        QSpinBox::down-button:hover {
            background-color: rgba(102, 204, 255, 200);
        }
    )");
    
    // 设置表头
    QStringList headers = {"香料种类", "是否使用", "是否绑定", "数量限制", "限制数量"};
    spiceTable->setHorizontalHeaderLabels(headers);
    
    // 设置表格属性
    spiceTable->verticalHeader()->setVisible(false);
    spiceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    spiceTable->setAlternatingRowColors(true);
    spiceTable->horizontalHeader()->setStretchLastSection(true);
    spiceTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    spiceTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 设置表格的大小策略和固定宽度
    spiceTable->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    spiceTable->setFixedWidth(540);  // 固定的总宽度，禁止修改
    
    // 设置具体的列宽
    spiceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    spiceTable->setColumnWidth(0, 120);   // 香料种类
    spiceTable->setColumnWidth(1, 70);    // 是否使用
    spiceTable->setColumnWidth(2, 70);    // 是否绑定
    spiceTable->setColumnWidth(3, 70);    // 数量限制
    spiceTable->setColumnWidth(4, 210);   // 限制数量 (调整为540-120-70-70-70=210)
    
    // 设置行高以适应大图标
    spiceTable->verticalHeader()->setDefaultSectionSize(50);
    
    // 香料种类列表
    QStringList spiceTypes = {
        "天然香料", "上等香料", "秘制香料", "极品香料", "皇室香料",
        "魔幻香料", "精灵香料", "天使香料", "圣灵香料"
    };
    
    // 填充表格内容
    for (int row = 0; row < 9; ++row) {
        // 香料种类标签（包含图标）
        QWidget* spiceWidget = new QWidget();
        QHBoxLayout* spiceLayout = new QHBoxLayout(spiceWidget);
        spiceLayout->setContentsMargins(5, 0, 5, 0);
        spiceLayout->setSpacing(5);
        
        // 香料图标
        QLabel* spiceIcon = new QLabel();
        QString iconPath = QString(":/images/spicesShow/%1.png").arg(spiceTypes[row]);
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull()) {
            spiceIcon->setPixmap(pixmap);
            // spiceIcon->setPixmap(pixmap.scaled(33, 33, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        spiceIcon->setAlignment(Qt::AlignCenter);
        
        // 香料名称
        QLabel* spiceLabel = new QLabel(spiceTypes[row]);
        spiceLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        spiceLabel->setStyleSheet(R"(
            font-weight: bold; 
            color: #003D7A;
            background-color: transparent;
        )");
        
        spiceLayout->addWidget(spiceIcon);
        spiceLayout->addWidget(spiceLabel);
        spiceLayout->addStretch();
        
        spiceTable->setCellWidget(row, 0, spiceWidget);
        
        // 是否使用复选框
        QCheckBox* useCheckBox = new QCheckBox();
        useCheckBox->setProperty("row", row);
        useCheckBox->setChecked(true);  // 默认勾选
        connect(useCheckBox, &QCheckBox::toggled, this, &StarryCard::onSpiceConfigChanged);
        
        QWidget* useWidget = new QWidget();
        useWidget->setFixedSize(70, 50);  // 明确设置容器大小
        QVBoxLayout* useLayout = new QVBoxLayout(useWidget);
        useLayout->setContentsMargins(0, 5, 0, 10);  // 上下边距控制
        useLayout->setSpacing(0);
        useLayout->addWidget(useCheckBox, 0, Qt::AlignHCenter);
        
        spiceTable->setCellWidget(row, 1, useWidget);
        
        // 是否绑定复选框
        QCheckBox* bindCheckBox = new QCheckBox();
        bindCheckBox->setProperty("row", row);
        connect(bindCheckBox, &QCheckBox::toggled, this, &StarryCard::onSpiceConfigChanged);
        
        QWidget* bindWidget = new QWidget();
        bindWidget->setFixedSize(70, 50);  // 明确设置容器大小
        QVBoxLayout* bindLayout = new QVBoxLayout(bindWidget);
        bindLayout->setContentsMargins(0, 5, 0, 10);  // 上下边距控制
        bindLayout->setSpacing(0);
        bindLayout->addWidget(bindCheckBox, 0, Qt::AlignHCenter);
        
        spiceTable->setCellWidget(row, 2, bindWidget);
        
        // 数量限制下拉框
        QComboBox* limitCombo = new QComboBox();
        limitCombo->addItem("无限制");
        limitCombo->addItem("限制");
        limitCombo->setProperty("row", row);
        connect(limitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &StarryCard::onSpiceConfigChanged);
        
        spiceTable->setCellWidget(row, 3, limitCombo);
        
        // 限制数量输入框
        QSpinBox* amountSpinBox = new QSpinBox();
        amountSpinBox->setMinimum(5);
        amountSpinBox->setMaximum(32765);
        amountSpinBox->setSingleStep(5);
        amountSpinBox->setSuffix(" 个");
        amountSpinBox->setValue(100);
        amountSpinBox->setEnabled(false);  // 默认禁用
        amountSpinBox->setProperty("row", row);
        
        // 自定义验证器，确保输入值为5的倍数（只在编辑完成时验证）
        connect(amountSpinBox, &QSpinBox::editingFinished, 
                [amountSpinBox]() {
                    int value = amountSpinBox->value();
                    if (value % 5 != 0) {
                        // 向下取整到5的倍数
                        int newValue = (value / 5) * 5;
                        amountSpinBox->blockSignals(true);
                        amountSpinBox->setValue(newValue);
                        amountSpinBox->blockSignals(false);
                    }
                });
        
        connect(amountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &StarryCard::onSpiceConfigChanged);
        
        spiceTable->setCellWidget(row, 4, amountSpinBox);
    }
    
    layout->addWidget(spiceTable);
    
    // 添加说明文字
    QLabel* tipLabel = new QLabel("提示：制卡方案会根据强化方案自动制作强化所需的卡片");
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setStyleSheet(R"(
        font-size: 12px; 
        color: #336699;
        background-color: rgba(125, 197, 255, 100);
        border-radius: 6px;
        padding: 8px;
        margin: 8px 0;
    )");
    layout->addWidget(tipLabel);
    
    // 加载现有配置
    loadSpiceConfig();
    
    return page;
}

void StarryCard::onSpiceConfigChanged()
{
    QObject* sender = QObject::sender();
    if (!sender) return;
    
    int row = sender->property("row").toInt();
    
    // 获取数量限制下拉框和限制数量输入框
    QComboBox* limitCombo = qobject_cast<QComboBox*>(spiceTable->cellWidget(row, 3));
    QSpinBox* amountSpinBox = qobject_cast<QSpinBox*>(spiceTable->cellWidget(row, 4));
    
    if (limitCombo && amountSpinBox) {
        // 根据数量限制选择启用/禁用限制数量输入框
        bool isLimited = (limitCombo->currentText() == "限制");
        amountSpinBox->setEnabled(isLimited);
        
        if (!isLimited) {
            amountSpinBox->setValue(0);
        } else if (amountSpinBox->value() == 0) {
            amountSpinBox->setValue(100);  // 默认值
        }
    }
    
    // 延迟保存配置，避免频繁I/O操作
    scheduleSpiceConfigSave();
}

void StarryCard::loadSpiceConfig()
{
    QFile file("spice_config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        addLog("香料配置文件不存在，使用默认配置", LogType::Warning);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        addLog("香料配置文件解析失败: " + error.errorString(), LogType::Error);
        return;
    }
    
    if (!doc.isObject()) {
        addLog("香料配置文件格式错误", LogType::Error);
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray spicesArray = root["spices"].toArray();
    
    if (spicesArray.size() != 9) {
        addLog("香料配置文件数据不完整", LogType::Warning);
        return;
    }
    
    // 加载配置到表格时阻止信号发射，避免触发保存
    for (int row = 0; row < 9 && row < spicesArray.size(); ++row) {
        QJsonObject spiceObj = spicesArray[row].toObject();
        
        // 设置是否使用状态
        QWidget* useWidget = spiceTable->cellWidget(row, 1);
        if (useWidget) {
            QCheckBox* useCheckBox = useWidget->findChild<QCheckBox*>();
            if (useCheckBox) {
                useCheckBox->blockSignals(true);
                useCheckBox->setChecked(spiceObj["used"].toBool(true));  // 默认为true
                useCheckBox->blockSignals(false);
            }
        }
        
        // 设置绑定状态
        QWidget* bindWidget = spiceTable->cellWidget(row, 2);
        if (bindWidget) {
            QCheckBox* bindCheckBox = bindWidget->findChild<QCheckBox*>();
            if (bindCheckBox) {
                bindCheckBox->blockSignals(true);
                bindCheckBox->setChecked(spiceObj["bound"].toBool());
                bindCheckBox->blockSignals(false);
            }
        }
        
        // 设置数量限制类型
        QComboBox* limitCombo = qobject_cast<QComboBox*>(spiceTable->cellWidget(row, 3));
        if (limitCombo) {
            QString limitType = spiceObj["limitType"].toString();
            int index = limitCombo->findText(limitType);
            if (index >= 0) {
                limitCombo->blockSignals(true);
                limitCombo->setCurrentIndex(index);
                limitCombo->blockSignals(false);
            }
        }
        
        // 设置限制数量
        QSpinBox* amountSpinBox = qobject_cast<QSpinBox*>(spiceTable->cellWidget(row, 4));
        if (amountSpinBox) {
            int amount = spiceObj["limitAmount"].toInt();
            amountSpinBox->blockSignals(true);
            amountSpinBox->setValue(amount);
            amountSpinBox->blockSignals(false);
            
            // 根据限制类型启用/禁用输入框
            bool isLimited = spiceObj["limitType"].toString() == "限制";
            amountSpinBox->setEnabled(isLimited);
        }
    }
    
    addLog("香料配置文件加载成功", LogType::Success);
}

void StarryCard::saveSpiceConfig()
{
    if (!spiceTable)
        return;

    QJsonObject root;
    QJsonArray spicesArray;

    static const QStringList spiceTypes = {
        "天然香料", "上等香料", "秘制香料", "极品香料", "皇室香料",
        "魔幻香料", "精灵香料", "天使香料", "圣灵香料"};

    // 添加边界检查
    int maxRows = qMin(9, spiceTypes.size());
    for (int row = 0; row < maxRows; ++row)
    {
        if (row >= spiceTypes.size())
        {
            qWarning() << "saveSpiceConfig: spiceTypes索引越界，跳过行" << row;
            break;
        }

        QJsonObject spiceObj;
        spiceObj["name"] = spiceTypes[row];

        // 获取是否使用状态
        QWidget *useWidget = spiceTable->cellWidget(row, 1);
        bool used = true; // 默认为true
        if (useWidget)
        {
            QCheckBox *useCheckBox = useWidget->findChild<QCheckBox *>();
            if (useCheckBox)
            {
                used = useCheckBox->isChecked();
            }
        }
        spiceObj["used"] = used;

        // 获取绑定状态
        QWidget *bindWidget = spiceTable->cellWidget(row, 2);
        bool bound = false;
        if (bindWidget)
        {
            QCheckBox *bindCheckBox = bindWidget->findChild<QCheckBox *>();
            if (bindCheckBox)
            {
                bound = bindCheckBox->isChecked();
            }
        }
        spiceObj["bound"] = bound;

        // 获取数量限制类型
        QComboBox *limitCombo = qobject_cast<QComboBox *>(spiceTable->cellWidget(row, 3));
        QString limitType = "无限制";
        if (limitCombo)
        {
            limitType = limitCombo->currentText();
        }
        spiceObj["limitType"] = limitType;

        // 获取限制数量
        QSpinBox *amountSpinBox = qobject_cast<QSpinBox *>(spiceTable->cellWidget(row, 4));
        int amount = 0;
        if (amountSpinBox)
        {
            amount = amountSpinBox->value();
        }
        spiceObj["limitAmount"] = amount;

        spiceObj["spiceLevel"] = row + 1;

        spicesArray.append(spiceObj);
    }

    root["spices"] = spicesArray;

    QJsonDocument doc(root);

    QFile file("spice_config.json");
    if (!file.open(QIODevice::WriteOnly))
    {
        addLog("无法保存香料配置文件", LogType::Error);
        return;
    }

    file.write(doc.toJson());
    file.close();

    // qDebug() << "香料配置已保存";
}

// 获取指定香料的绑定状态配置（只读操作，从配置文件读取）
QPair<bool, bool> StarryCard::getSpiceBindingConfig(const QString& spiceType) const
{
    // 从配置文件中读取香料配置（只读操作）
    QFile file("spice_config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "香料配置文件不存在，使用默认配置（未绑定）";
        return qMakePair(false, true); // 默认只接受未绑定状态
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "香料配置文件解析失败，使用默认配置（未绑定）";
        return qMakePair(false, true);
    }
    
    if (!doc.isObject()) {
        qDebug() << "香料配置文件格式错误，使用默认配置（未绑定）";
        return qMakePair(false, true);
    }
    
    QJsonObject root = doc.object();
    QJsonArray spicesArray = root["spices"].toArray();
    
    // 查找指定香料的配置
    bool bindingRequired = false; // 默认不绑定
    bool found = false;
    
    for (int i = 0; i < spicesArray.size(); ++i) {
        QJsonObject spiceObj = spicesArray[i].toObject();
        QString name = spiceObj["name"].toString();
        
        if (name == spiceType) {
            bindingRequired = spiceObj["bound"].toBool(false); // 默认false
            found = true;
            break;
        }
    }
    
    if (!found) {
        qDebug() << "未在配置文件中找到香料类型:" << spiceType << "，使用默认配置（未绑定）";
        return qMakePair(false, true);
    }
    
    // 根据绑定配置返回相应的参数
    if (bindingRequired) {
        // 如果配置要求绑定，则只接受绑定状态
        qDebug() << QString("香料 %1 配置要求绑定状态").arg(spiceType);
        return qMakePair(true, false);  // spice_bound=true, spice_unbound=false
    } else {
        // 如果配置要求不绑定，则只接受未绑定状态
        qDebug() << QString("香料 %1 配置要求未绑定状态").arg(spiceType);
        return qMakePair(false, true);  // spice_bound=false, spice_unbound=true
    }
}

// 计算智能香料分配
QList<QPair<QString, int>> StarryCard::calculateSpiceAllocation(int totalCardCount)
{
    QList<QPair<QString, int>> allocation;
    
    // 从配置文件中读取香料配置
    QFile file("spice_config.json");
    if (!file.open(QIODevice::ReadOnly)) {
        addLog("香料配置文件不存在，无法进行智能分配", LogType::Error);
        return allocation;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        addLog("香料配置文件解析失败，无法进行智能分配", LogType::Error);
        return allocation;
    }
    
    QJsonObject root = doc.object();
    QJsonArray spicesArray = root["spices"].toArray();
    
    // 第一步：收集所有启用的香料及其配置
    struct SpiceConfig {
        QString name;
        bool used;
        bool isLimited;
        int limitAmount;
        int clicksNeeded; // 该香料需要的点击次数
    };
    
    QList<SpiceConfig> enabledSpices;
    int totalLimitedClicks = 0; // 限制类型香料的总点击次数
    
    for (int i = 0; i < spicesArray.size(); ++i) {
        QJsonObject spiceObj = spicesArray[i].toObject();
        bool used = spiceObj["used"].toBool(false);
        
        if (!used) continue; // 跳过未启用的香料
        
        SpiceConfig config;
        config.name = spiceObj["name"].toString();
        config.used = used;
        config.isLimited = (spiceObj["limitType"].toString() == "限制");
        config.limitAmount = spiceObj["limitAmount"].toInt(0);
        config.clicksNeeded = 0;
        
        if (config.isLimited) {
            // 限制类型：计算点击次数 = limitAmount / 5（向下取整）
            config.clicksNeeded = qMin(config.limitAmount / 5, 32765 / 5); // 防止超过最大限制
            totalLimitedClicks += config.clicksNeeded;
        }
        
        enabledSpices.append(config);
    }
    
    if (enabledSpices.isEmpty()) {
        addLog("没有启用的香料，无法进行制卡", LogType::Warning);
        return allocation;
    }
    
    // 第二步：智能分配逻辑 - 区分真正受限和充足的香料
    addLog("开始智能分配香料...", LogType::Info);
    
    // 计算如果所有香料均分，每个香料应该分配多少次
    int totalSpiceCount = enabledSpices.size();
    int averageClicksPerSpice = totalCardCount / totalSpiceCount;
    
    addLog(QString("总香料数: %1, 平均每种香料: %2次").arg(totalSpiceCount).arg(averageClicksPerSpice), LogType::Info);
    
    // 分类香料：真正受限的香料 vs 充足的香料（无限制 + 限制量充足）
    QList<SpiceConfig> trulyLimitedSpices;  // 真正受限的香料
    QStringList abundantSpices;             // 充足的香料
    
    for (const auto& spice : enabledSpices) {
        if (spice.isLimited) {
            // 如果限制量小于平均分配量，才算真正受限
            if (spice.clicksNeeded < averageClicksPerSpice) {
                trulyLimitedSpices.append(spice);
                addLog(QString("真正受限香料: %1 (限制%2次 < 平均%3次)")
                       .arg(spice.name).arg(spice.clicksNeeded).arg(averageClicksPerSpice), LogType::Info);
            } else {
                // 限制量充足，视为无限制
                abundantSpices.append(spice.name);
                addLog(QString("充足香料: %1 (限制%2次 >= 平均%3次)")
                       .arg(spice.name).arg(spice.clicksNeeded).arg(averageClicksPerSpice), LogType::Info);
            }
        } else {
            // 无限制香料
            abundantSpices.append(spice.name);
            addLog(QString("无限制香料: %1").arg(spice.name), LogType::Info);
        }
    }
    
    // 第一步：先分配真正受限的香料
    int usedClicks = 0;
    for (const auto& spice : trulyLimitedSpices) {
        allocation.append(qMakePair(spice.name, spice.clicksNeeded));
        usedClicks += spice.clicksNeeded;
        addLog(QString("分配受限香料 %1: %2次").arg(spice.name).arg(spice.clicksNeeded), LogType::Info);
    }
    
    // 第二步：剩余的制卡需求由充足香料均分
    int remainingClicks = totalCardCount - usedClicks;
    if (remainingClicks > 0 && !abundantSpices.isEmpty()) {
        addLog(QString("剩余制卡需求: %1次，由%2种充足香料均分").arg(remainingClicks).arg(abundantSpices.size()), LogType::Info);
        
        int clicksPerAbundantSpice = remainingClicks / abundantSpices.size();
        int extraClicks = remainingClicks % abundantSpices.size();
        
        for (int i = 0; i < abundantSpices.size(); ++i) {
            int clicks = clicksPerAbundantSpice;
            if (i < extraClicks) {
                clicks++; // 前几个香料多分配一次
            }
            
            if (clicks > 0) {
                allocation.append(qMakePair(abundantSpices[i], clicks));
                addLog(QString("分配充足香料 %1: %2次").arg(abundantSpices[i]).arg(clicks), LogType::Info);
            }
        }
    } else if (remainingClicks > 0) {
        addLog(QString("警告：还有%1次制卡需求未分配，无充足香料可用").arg(remainingClicks), LogType::Warning);
    }
    
    // 第三步：按优先级排序分配结果（从圣灵香料到天然香料）
    QStringList spicePriorityOrder = {
        "圣灵香料", "天使香料", "精灵香料", "魔幻香料", "皇室香料",
        "极品香料", "秘制香料", "上等香料", "天然香料"
    };
    
    QList<QPair<QString, int>> sortedAllocation;
    
    // 按优先级顺序重新排列分配结果
    for (const QString& prioritySpice : spicePriorityOrder) {
        for (const auto& pair : allocation) {
            if (pair.first == prioritySpice) {
                sortedAllocation.append(pair);
                break;
            }
        }
    }
    
    // 添加任何不在优先级列表中的香料（如"无香料"）
    for (const auto& pair : allocation) {
        bool found = false;
        for (const auto& sortedPair : sortedAllocation) {
            if (sortedPair.first == pair.first) {
                found = true;
                break;
            }
        }
        if (!found) {
            sortedAllocation.append(pair);
        }
    }
    
    // 第四步：输出排序后的分配结果
    addLog("===== 香料分配结果 ======", LogType::Info);
    int totalAllocated = 0;
    for (const auto& pair : sortedAllocation) {
        addLog(QString("%1: %2次").arg(pair.first).arg(pair.second), LogType::Info);
        totalAllocated += pair.second;
    }
    addLog(QString("总计: %1次 (目标: %2次)").arg(totalAllocated).arg(totalCardCount), LogType::Info);
    addLog("====================", LogType::Info);
    
    return sortedAllocation;
}

// ========== EnhancementWorker 类实现 ==========

EnhancementWorker::EnhancementWorker(StarryCard* parent)
    : QObject(nullptr), m_parent(parent)
{
}

void EnhancementWorker::startEnhancement()
{
    if (!m_parent->hwndGame || !IsWindow(m_parent->hwndGame)) {
        emit showWarningMessage("错误", "请先绑定有效的游戏窗口！");
        return;
    }

    if (!m_parent->isEnhancing) {
        // 卡片类型已经在主线程中预加载到requiredCardTypes
        if (m_parent->requiredCardTypes.isEmpty()) {
            emit showWarningMessage("警告", "配置文件中未找到有效的卡片类型配置！\n请先在卡片设置中配置主卡和副卡类型。");
            emit logMessage("强化流程启动失败：未找到有效的卡片类型配置", LogType::Error);
            return;
        }

        // 最高强化等级和最低强化等级已经在主线程中设置到成员变量
        // 全局强化配置已经在主线程中预加载
        
        // 分析强化配置，提取需要制作的卡片信息
        analyzeEnhancementConfigForCardProduce();
        
        // 创建本地副本避免跨线程访问QStringList
        QStringList cardTypesCopy = m_parent->requiredCardTypes;
        emit logMessage(QString("强化流程准备就绪，将识别以下卡片类型: %1").arg(cardTypesCopy.join(", ")), LogType::Info);

        m_parent->isEnhancing = true;
        emit logMessage("开始强化流程", LogType::Success);

        // 关闭健康提示
        m_parent->closeHealthTip(1);

        while(!m_parent->goToPage(StarryCard::PageType::CardEnhance) && m_parent->isEnhancing)
        {
            emit logMessage("未找到卡片强化页面，尝试刷新游戏窗口...", LogType::Warning);
            // 刷新游戏窗口
            if(!m_parent->refreshGameWindow())
            {
                m_parent->isEnhancing = false;
                emit showWarningMessage("错误", "刷新游戏窗口失败，强化已停止！");
                emit logMessage("刷新游戏窗口失败，强化已停止！", LogType::Error);
                emit enhancementFinished();
                return;
            }
            // 关闭健康提示
            m_parent->closeHealthTip();
            threadSafeSleep(1000);
        }
        
        performEnhancement();
    }
}

void EnhancementWorker::performEnhancement()
{
    if (!m_parent->hwndGame || !IsWindow(m_parent->hwndGame)) {
        m_parent->isEnhancing = false;
        emit showWarningMessage("错误", "目标窗口已失效，强化已停止！");
        emit logMessage("目标窗口已失效，强化已停止！", LogType::Error);
        emit enhancementFinished();
        return;
    }

    // 创建本地副本避免跨线程访问QStringList
    QStringList cardTypesCopy = m_parent->requiredCardTypes;

    // 重置卡片背包滚动条位置
    m_parent->resetScrollBar();
    
    // 获取滚动条长度并计算单行滚动长度
    QImage screenshot = m_parent->captureWindowByHandle(m_parent->hwndGame, "主页面");
    int scrollBarLength = m_parent->getLengthOfScrollBar(screenshot);
    int singleLineScrollLength = static_cast<int>(scrollBarLength / 8.0 - 1);  // 计算公式：滚动条长度 / 8 - 1，向下取整
    int singlePageScrollLength = static_cast<int>(scrollBarLength * 7 / 8 + 2); // 单页滚动长度
    int scrollBarPosition = m_parent->getPositionOfScrollBar(screenshot);

    QVector<CardInfo> cardVector;

    // 记录滚动条信息
    emit logMessage(QString("滚动条长度: %1, 单行滚动长度: %2").arg(scrollBarLength).arg(singleLineScrollLength), LogType::Info);

    while (m_parent->isEnhancing)
    {
        // 单卡强化时，将滚动条拖动到第一张低于最高等级的卡片位置
        if(cardTypesCopy.length() == 1)
        {
            int maxLevel = min(m_parent->maxEnhancementLevel, 10); // 10星及以上的卡片会沉底，所以最大等级为10
            emit logMessage(QString("最高强化等级: %1").arg(maxLevel), LogType::Info);
            int i = 0;
            while (i < 8)
            {
                i++;
                if(i == 8)
                {
                    m_parent->isEnhancing = false;
                    emit logMessage("未找到目标卡片，强化已停止！", LogType::Error);
                    break;
                }
                QImage screenshot = m_parent->captureWindowByHandle(m_parent->hwndGame, "主页面");
                cardVector = m_parent->cardRecognizer->recognizeCards(screenshot, cardTypesCopy);

                // 未找到卡片，或者cardVector最后一张卡片位于(6,6)且大于最高等级，滚动到下一页
                if (cardVector.empty() || (cardVector.back().row == 6 && cardVector.back().col == 6 && cardVector.back().level >= maxLevel))
                {
                    m_parent->fastMouseDrag(910, 120 + scrollBarPosition, singlePageScrollLength, true);
                    emit logMessage(QString("滚动到下一页: %1").arg(singlePageScrollLength), LogType::Info);
                    while (m_parent->getPositionOfScrollBar(screenshot) == scrollBarPosition)
                    {
                        threadSafeSleep(100);
                        screenshot = m_parent->captureWindowByHandle(m_parent->hwndGame, "主页面");
                    }
                    scrollBarPosition = m_parent->getPositionOfScrollBar(screenshot);
                }
                else
                {
                    for (const auto &card : cardVector)
                    {
                        if (card.level < maxLevel)
                        {
                            int row = card.row; // 卡片所在行
                            if(row == 0) // 第一行卡片为最高等级-1，直接跳出循环
                            {
                                break;
                            }
                            int scrollLength = static_cast<int>(scrollBarLength / 8 * row);
                            emit logMessage(QString("滚动到第%1行: %2").arg(row).arg(scrollLength), LogType::Info);
                            m_parent->fastMouseDrag(910, 120 + scrollBarPosition, scrollLength, true);
                            while (m_parent->getPositionOfScrollBar(screenshot) == scrollBarPosition)
                            {
                                threadSafeSleep(100);
                                screenshot = m_parent->captureWindowByHandle(m_parent->hwndGame, "主页面");
                            }
                            scrollBarPosition = m_parent->getPositionOfScrollBar(screenshot);
                            emit logMessage(QString("更新滚动条位置: %1").arg(scrollBarPosition), LogType::Info);
                            cardVector = m_parent->cardRecognizer->recognizeCards(screenshot, cardTypesCopy); // 重新识别卡片
                            break;
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            emit logMessage("多卡强化，未实现", LogType::Error);
            m_parent->isEnhancing = false;
            break;
        }
        
        
        if (cardVector.empty())
        {
            m_parent->isEnhancing = false;
            emit logMessage("未找到目标卡片，强化已停止！", LogType::Error);
            break;
        }
        

        // 调用强化处理方法
        if (!performEnhancementOnce(cardVector) && m_parent->isEnhancing)
        {
            // 返回值为FALSE，且未停止强化，说明未找到可以强化的卡片，启动制卡流程
            emit logMessage("未找到可以强化的卡片，启动制卡流程", LogType::Info);
            // 此处待实现制卡流程
            m_parent->isEnhancing = false;
        }
    }

    m_parent->isEnhancing = false;
    emit enhancementFinished();
}

BOOL EnhancementWorker::performEnhancementOnce(const QVector<CardInfo>& cardVector)
{
    emit logMessage("开始分析卡片强化配置", LogType::Info);
    
    // 从最高等级开始，遍历每个强化等级
    for (int level = m_parent->maxEnhancementLevel; level >= m_parent->minEnhancementLevel; --level) {
        // 检查是否有对应的配置
        if (!g_enhancementConfig.hasLevelConfig(level - 1, level)) {
            emit logMessage(QString("跳过等级 %1-%2：无配置").arg(level - 1).arg(level), LogType::Info);
            continue;
        }
        
        auto levelConfig = g_enhancementConfig.getLevelConfig(level - 1, level);
        emit logMessage(QString("检查等级 %1-%2 的配置").arg(level - 1).arg(level), LogType::Info);
        
        // 检查是否有足够的卡片满足要求
        QVector<CardInfo> selectedCards;
        bool hasEnoughCards = true;
        
        // 1. 检查主卡（目标等级-1的星级）
        CardInfo mainCard;
        bool foundMainCard = false;
        
        for (const auto& card : cardVector) {
            if (card.name == levelConfig.mainCardType && 
                card.level == level - 1) {
                // 检查绑定状态匹配
                if ((levelConfig.mainCardBound && card.isBound) || 
                    (levelConfig.mainCardUnbound && !card.isBound)) {
                    mainCard = card;
                    foundMainCard = true;
                    break;
                }
            }
        }
        
        if (!foundMainCard) {
            // emit logMessage(QString("等级 %1-%2：缺少主卡 %3 (%4星)").arg(level - 1).arg(level)
            //       .arg(levelConfig.mainCardType).arg(level - 1), LogType::Warning);
            hasEnoughCards = false;
            continue;
        } else {
            selectedCards.push_back(mainCard);
            // emit logMessage(QString("找到主卡：%1 (%2星, %3)").arg(mainCard.name)
            //       .arg(mainCard.level).arg(mainCard.isBound ? "绑定" : "未绑定"), LogType::Success);
        }
        
        // 2. 检查副卡
        QVector<int> requiredSubcards;
        if (levelConfig.subcard1 >= 0) requiredSubcards.push_back(levelConfig.subcard1);
        if (levelConfig.subcard2 >= 0) requiredSubcards.push_back(levelConfig.subcard2);
        if (levelConfig.subcard3 >= 0) requiredSubcards.push_back(levelConfig.subcard3);
        
        QVector<CardInfo> availableSubcards;
        for (const auto& card : cardVector) {
            if (card.name == levelConfig.subCardType && 
                card.centerPosition != mainCard.centerPosition) { // 不能是主卡
                // 检查绑定状态匹配
                if ((levelConfig.subCardBound && card.isBound) || 
                    (levelConfig.subCardUnbound && !card.isBound)) {
                    availableSubcards.push_back(card);
                }
            }
        }
        
        // 按星级分组副卡
        std::map<int, QVector<CardInfo>> subcardsByLevel;
        for (const auto& card : availableSubcards) {
            subcardsByLevel[card.level].push_back(card);
        }
        
        // 检查是否有足够的副卡
        QVector<CardInfo> selectedSubcards;
        for (int requiredLevel : requiredSubcards) {
            if (subcardsByLevel[requiredLevel].empty()) {
                // emit logMessage(QString("等级 %1-%2：缺少副卡 %3 (%4星)").arg(level - 1).arg(level)
                //       .arg(levelConfig.subCardType).arg(requiredLevel), LogType::Warning);
                hasEnoughCards = false;
                if(level - 1 > requiredLevel)
                {
                    // 如果缺少的副卡不是主卡同等级的卡，尝试强化缺少的副卡
                    // emit logMessage(QString("%1 星主卡充足，尝试强化:%2 星副卡").arg(level - 1).arg(requiredLevel), LogType::Info);
                    level = requiredLevel + 1;
                }
                break;
            } else {
                selectedSubcards.push_back(subcardsByLevel[requiredLevel][0]);
                subcardsByLevel[requiredLevel].erase(subcardsByLevel[requiredLevel].begin());
                // emit logMessage(QString("找到副卡：%1 (%2星, %3)").arg(selectedSubcards.back().name)
                //       .arg(selectedSubcards.back().level).arg(selectedSubcards.back().isBound ? "绑定" : "未绑定"), LogType::Success);
            }
        }
        
        if (hasEnoughCards) {
            emit logMessage(QString("等级 %1-%2：卡片齐备，开始强化").arg(level - 1).arg(level), LogType::Success);
            
            // 3. 点击选中的卡片
            // 首先点击主卡
            m_parent->leftClickDPI(m_parent->hwndGame, mainCard.centerPosition.x(), mainCard.centerPosition.y());
            // emit logMessage(QString("点击主卡：(%1, %2)").arg(mainCard.centerPosition.x()).arg(mainCard.centerPosition.y()), LogType::Info);
            
            // 然后点击副卡
            for (const auto& subcard : selectedSubcards) {
                m_parent->leftClickDPI(m_parent->hwndGame, subcard.centerPosition.x(), subcard.centerPosition.y());
                // emit logMessage(QString("点击副卡：(%1, %2)").arg(subcard.centerPosition.x()).arg(subcard.centerPosition.y()), LogType::Info);
            }
            
            // 4. 检查是否需要四叶草
            if (levelConfig.clover != "无" && levelConfig.clover != "") {
                emit logMessage(QString("检查四叶草：%1").arg(levelConfig.clover), LogType::Info);
                
                QPair<bool, bool> cloverResult = m_parent->recognizeClover(levelConfig.clover, 
                                                                levelConfig.cloverBound, 
                                                                levelConfig.cloverUnbound);
                if (cloverResult.first) {
                    emit logMessage(QString("成功添加四叶草：%1 (%2)").arg(levelConfig.clover)
                          .arg(cloverResult.second ? "绑定" : "未绑定"), LogType::Success);
                } else {
                    emit logMessage(QString("未找到四叶草：%1").arg(levelConfig.clover), LogType::Warning);
                    m_parent->isEnhancing = false;
                    emit showWarningMessage("错误", "未找到四叶草，强化已停止！");
                    return FALSE;
                }
            }
            
            emit logMessage(QString("等级 %1-%2 的强化材料准备完成").arg(level - 1).arg(level), LogType::Success);

            // 等待强化按钮就绪
            for (int i = 0; i < 100 && m_parent->isEnhancing; i++)
            {
                QImage screenshot = m_parent->captureWindowByHandle(m_parent->hwndGame, "主页面");

                if (m_parent->checkSynHousePosState(screenshot, m_parent->ENHANCE_BUTTON_POS, "enhanceButtonReady"))
                {
                    emit logMessage("强化按钮已就绪，点击强化按钮", LogType::Success);
                    m_parent->leftClickDPI(m_parent->hwndGame, 285, 435); // 点击强化按钮
                    break;
                }
                else if(i == 99)
                {
                    m_parent->isEnhancing = false;
                    emit showWarningMessage("错误", "强化按钮异常，强化已停止！");
                    return FALSE;
                }
                threadSafeSleep(30);
            }

            // 等待副卡位置为空
            for (int i = 0; i < 100 && m_parent->isEnhancing; i++)
            {
                QImage screenshot = m_parent->captureWindowByHandle(m_parent->hwndGame, "主页面");
                if (m_parent->checkSynHousePosState(screenshot, m_parent->SUB_CARD_POS, "subCardEmpty"))
                {
                    // emit logMessage("副卡位置为空，强化完成，等待强化结果", LogType::Success);
                    break;
                }
                else if(i == 99)
                {
                    m_parent->isEnhancing = false;
                    emit showWarningMessage("错误", "副卡位置异常，强化已停止！");
                    return FALSE;
                }
                threadSafeSleep(50);
            }

            // 强化完成后清空主卡位置
            m_parent->leftClickDPI(m_parent->hwndGame, 288, 350); // 点击主卡位置卸下主卡
            for (int i = 0; i < 100 && m_parent->isEnhancing; i++)
            {
                QImage screenshot = m_parent->captureWindowByHandle(m_parent->hwndGame, "主页面");
                if (m_parent->checkSynHousePosState(screenshot, m_parent->MAIN_CARD_POS, "mainCardEmpty"))
                {
                    // emit logMessage(QString("主卡位置为空，卸卡完成，%1-%2星强化完成").arg(level - 1).arg(level), LogType::Success);
                    return TRUE;
                }
                else if(i == 99)
                {
                    m_parent->isEnhancing = false;
                    emit showWarningMessage("错误", "主卡位置异常，强化已停止！");
                    return FALSE;
                }
                threadSafeSleep(50);
            }
        }
    }

    // 未找到可以强化的卡片，启动制卡流程
    return FALSE;
}

// BOOL EnhancementWorker::performCardProduce(const QVector<CardInfo>& cardVector)
// {
//     goToPage(PageType::CardProduce); // 进入制卡页面

//     threadSafeSleep(100);

//   }

void EnhancementWorker::analyzeEnhancementConfigForCardProduce()
{
    // 清空之前的制卡配置
    g_cardProduceConfig.clear();
    
    emit logMessage("开始分析强化配置，提取制卡需求...", LogType::Info);
    
    // 从最高等级开始，遍历每个强化等级
    for (int level = m_parent->maxEnhancementLevel; level >= m_parent->minEnhancementLevel; --level) {
        // 获取当前等级的配置
        auto levelConfig = g_enhancementConfig.getLevelConfig(level - 1, level);
        
        // 检查副卡配置
        QVector<int> subcardLevels;
        if (levelConfig.subcard1 >= 0) subcardLevels.append(levelConfig.subcard1);
        if (levelConfig.subcard2 >= 0) subcardLevels.append(levelConfig.subcard2);
        if (levelConfig.subcard3 >= 0) subcardLevels.append(levelConfig.subcard3);
        
        // 如果有副卡需求且副卡类型不为"无"
        if (!subcardLevels.isEmpty() && levelConfig.subCardType != "无" && !levelConfig.subCardType.isEmpty()) {
            for (int subcardLevel : subcardLevels) {
                // 先检查是否已存在，避免重复日志
                CardProduceConfig::ProduceItem newItem(
                    levelConfig.subCardType,
                    subcardLevel,
                    levelConfig.subCardBound,
                    levelConfig.subCardUnbound
                );
                
                bool itemExists = false;
                for (const auto& existingItem : g_cardProduceConfig.produceItems) {
                    if (existingItem == newItem) {
                        itemExists = true;
                        break;
                    }
                }
                
                g_cardProduceConfig.addProduceItem(
                    levelConfig.subCardType,
                    subcardLevel,
                    levelConfig.subCardBound,
                    levelConfig.subCardUnbound
                );
                
                if (!itemExists) {
                    emit logMessage(QString("添加制卡需求: %1 %2星 (绑定:%3, 不绑:%4) - 来自等级%5-%6强化配置")
                        .arg(levelConfig.subCardType)
                        .arg(subcardLevel)
                        .arg(levelConfig.subCardBound ? "是" : "否")
                        .arg(levelConfig.subCardUnbound ? "是" : "否")
                        .arg(level - 1).arg(level), LogType::Info);
                } else {
                    emit logMessage(QString("跳过重复的制卡需求: %1 %2星 (绑定:%3, 不绑:%4)")
                        .arg(levelConfig.subCardType)
                        .arg(subcardLevel)
                        .arg(levelConfig.subCardBound ? "是" : "否")
                        .arg(levelConfig.subCardUnbound ? "是" : "否"), LogType::Info);
                }
            }
        }
        
        // 检查主卡配置（如果需要特定等级的主卡）
        // 注意：通常主卡是从背包中选择现有的，但某些情况下可能需要制作特定等级的主卡
        if (levelConfig.mainCardType != "无" && !levelConfig.mainCardType.isEmpty()) {
            // 这里可以根据具体需求决定是否需要制作主卡
            // 例如，如果主卡需要特定等级且背包中没有，则需要制作
            // 暂时注释掉，可根据实际需求启用
            /*
            g_cardProduceConfig.addProduceItem(
                levelConfig.mainCardType,
                level - 1,  // 主卡通常比目标等级低1
                levelConfig.mainCardBound,
                levelConfig.mainCardUnbound
            );
            */
        }
    }
    
    // 最终去重和排序（确保数据一致性）
    g_cardProduceConfig.removeDuplicates();
    g_cardProduceConfig.sortProduceItems();
    
    // 输出制卡需求汇总
    if (g_cardProduceConfig.produceItems.isEmpty()) {
        emit logMessage("未发现制卡需求", LogType::Warning);
    } else {
        emit logMessage(QString("制卡需求分析完成，共需制作 %1 个不同的制卡项目，涉及 %2 种不同类型的卡片")
            .arg(g_cardProduceConfig.produceItems.size())
            .arg(g_cardProduceConfig.getUniqueCardTypes().size()), LogType::Success);
        
        // 详细显示每个制卡需求
        emit logMessage("详细制卡需求列表:", LogType::Info);
        for (int i = 0; i < g_cardProduceConfig.produceItems.size(); ++i) {
            const auto& item = g_cardProduceConfig.produceItems[i];
            emit logMessage(QString("  %1. %2 %3星 (绑定:%4, 不绑:%5)")
                .arg(i + 1)
                .arg(item.cardType)
                .arg(item.targetLevel)
                .arg(item.bound ? "是" : "否")
                .arg(item.unbound ? "是" : "否"), LogType::Info);
        }
        
        // 按卡片类型分组显示制卡需求汇总
        emit logMessage("按卡片类型分组汇总:", LogType::Info);
        QStringList uniqueTypes = g_cardProduceConfig.getUniqueCardTypes();
        for (const QString& cardType : uniqueTypes) {
            QVector<CardProduceConfig::ProduceItem> items = g_cardProduceConfig.getProduceItemsByType(cardType);
            QStringList levelInfo;
            for (const auto& item : items) {
                QString bindInfo = "";
                if (item.bound && item.unbound) {
                    bindInfo = "(绑定+不绑)";
                } else if (item.bound) {
                    bindInfo = "(仅绑定)";
                } else if (item.unbound) {
                    bindInfo = "(仅不绑)";
                }
                levelInfo.append(QString("%1星%2").arg(item.targetLevel).arg(bindInfo));
            }
            emit logMessage(QString("- %1: %2").arg(cardType).arg(levelInfo.join(", ")), LogType::Info);
        }
    }
}

void EnhancementWorker::threadSafeSleep(int ms)
{
    QThread::msleep(ms);
}

// ==================== 滚动条拖动相关 ====================

void StarryCard::fastMouseDrag(int startX, int startY, int distance, bool downward)
{
    if (!hwndGame || !IsWindow(hwndGame)) {
        qDebug() << "无效的窗口句柄，无法执行快速鼠标拖动";
        return;
    }

    // 获取DPI
    int DPI = getDPIFromDC();

    double scaleFactor = static_cast<double>(DPI) / 96.0;
    
    // 计算结束坐标（仅垂直移动）
    int endX = startX;
    int endY = downward ? (startY + distance) : (startY - distance);
    
    // 计算DPI缩放后的坐标
    POINT startPoint = {static_cast<int>(startX * scaleFactor), static_cast<int>(startY * scaleFactor)};
    POINT endPoint = {static_cast<int>(endX * scaleFactor), static_cast<int>(endY * scaleFactor)};
    
    // 构造LPARAM参数
    LPARAM startLParam = MAKELPARAM(startPoint.x, startPoint.y);
    LPARAM endLParam = MAKELPARAM(endPoint.x, endPoint.y);
    
    // 快速拖动：最少步骤完成拖动
    // 步骤1: 鼠标按下
    PostMessage(hwndGame, WM_LBUTTONDOWN, MK_LBUTTON, startLParam);
    
    // 步骤2: 移动到目标位置
    PostMessage(hwndGame, WM_MOUSEMOVE, MK_LBUTTON, endLParam);
    
    // 步骤3: 鼠标释放
    PostMessage(hwndGame, WM_LBUTTONUP, 0, endLParam);
}

// 重置滚动条到顶端
BOOL StarryCard::resetScrollBar()
{
    leftClickDPI(hwndGame, 910, 112);
    int i = 0;
    QRect scrollTopRoi = QRect(902, 98, 16, 16);
    while(i < 100)
    {
        if(checkSynHousePosState(captureWindowByHandle(hwndGame, "主页面"), scrollTopRoi, "enhanceScrollTop"))
        {
            return TRUE;
        }
        sleepByQElapsedTimer(50);
        i++;
    }
    return FALSE;
}

// 获取滚动条长度
int StarryCard::getLengthOfScrollBar(QImage screenshot)
{
    if(screenshot.width() < 950 || screenshot.height() < 596)
    {
        qDebug() << "截图尺寸异常，无法获取滚动条长度";
        return 0;
    }

    // 目标颜色 #0A486F (RGB: 10, 72, 111)
    QColor targetColor(10, 72, 111);
    int targetRgb = targetColor.rgb();

    for(int i = 0; i < 450; i++)
    {
        QColor pixelColor = screenshot.pixelColor(903, 108 + i);
        if(pixelColor.rgb() == targetRgb)
        {
            return i;
        }
    }
    return 0;
}

int StarryCard::getPositionOfScrollBar(QImage screenshot)
{
    if(screenshot.width() < 950 || screenshot.height() < 596)
    {
        qDebug() << "截图尺寸异常，无法获取滚动条长度";
        return 0;
    }

    // 目标颜色 #0A486F (RGB: 10, 72, 111)
    QColor targetColor(10, 72, 111);
    int targetRgb = targetColor.rgb();

    for(int i = 0; i < 450; i++)
    {
        QColor pixelColor = screenshot.pixelColor(903, 108 + i);
        if(pixelColor.rgb() != targetRgb)
        {
            return i;
        }
    }
}

// ================== 制卡功能实现 ==================

QStringList StarryCard::getSelectedSpices()
{
    if (!spiceTable) {
        addLog("香料配置表格未初始化", LogType::Error);
        return QStringList();
    }
    
    // 香料优先级顺序（从高到低）
    QStringList spicePriorityOrder = {
        "圣灵香料", "天使香料", "精灵香料", "魔幻香料", "皇室香料",
        "极品香料", "秘制香料", "上等香料", "天然香料"
    };
    
    // 表格中的香料顺序
    QStringList tableSpiceOrder = {
        "天然香料", "上等香料", "秘制香料", "极品香料", "皇室香料",
        "魔幻香料", "精灵香料", "天使香料", "圣灵香料"
    };
    
    QStringList selectedSpices;
    
    // 检查每个香料是否被勾选
    for (int row = 0; row < 9 && row < tableSpiceOrder.size(); ++row) {
        // 获取"是否使用"状态（第1列）
        QWidget* useWidget = spiceTable->cellWidget(row, 1);
        bool used = false;
        if (useWidget) {
            QCheckBox* useCheckBox = useWidget->findChild<QCheckBox*>();
            if (useCheckBox) {
                used = useCheckBox->isChecked();
            }
        }
        
        if (used) {
            selectedSpices.append(tableSpiceOrder[row]);
        }
    }
    
    // 按优先级排序
    QStringList sortedSelectedSpices;
    for (const QString& prioritySpice : spicePriorityOrder) {
        if (selectedSpices.contains(prioritySpice)) {
            sortedSelectedSpices.append(prioritySpice);
        }
    }
    
    addLog(QString("已选择香料（按优先级排序）: %1").arg(sortedSelectedSpices.join(", ")), LogType::Info);
    return sortedSelectedSpices;
}

bool StarryCard::recognizeMakeButton()
{
    if (!hwndGame || !IsWindow(hwndGame)) {
        addLog("游戏窗口无效，无法识别制作按钮", LogType::Error);
        return false;
    }
    
    // 截取游戏窗口
    QImage screenshot = captureWindowByHandle(hwndGame, "制作按钮识别");
    if (screenshot.isNull()) {
        addLog("截取游戏窗口失败", LogType::Error);
        return false;
    }
    
    // 截取制作按钮区域：以(260,416)为左顶点，20*20区域
    QRect buttonArea(260, 416, 20, 20);
    QImage buttonImage = screenshot.copy(buttonArea);
    
    if (buttonImage.isNull()) {
        addLog("截取制作按钮区域失败", LogType::Error);
        return false;
    }
    
    // 静态加载模板图片（避免重复加载）
    static QImage makeTemplate(":/images/position/(260,416)制作.png");
    static QImage makeBrightTemplate(":/images/position/(260,416)制作亮.png");
    static QString makeHash = calculateImageHash(makeTemplate);
    static QString makeBrightHash = calculateImageHash(makeBrightTemplate);
    
    if (makeTemplate.isNull() || makeBrightTemplate.isNull()) {
        addLog("加载制作按钮模板失败", LogType::Error);
        return false;
    }
    
    // 计算当前按钮图像的哈希值
    QString currentHash = calculateImageHash(buttonImage);
    
    // 计算相似度
    double makeSimilarity = calculateHashSimilarity(currentHash, makeHash);
    double makeBrightSimilarity = calculateHashSimilarity(currentHash, makeBrightHash);
    
    // 如果任一模板相似度大于0.8，则认为识别成功
    bool recognized = (makeSimilarity > 0.8 || makeBrightSimilarity > 0.8);
    
    if (recognized) {
        addLog("制作按钮识别成功", LogType::Success);
    } else {
        addLog(QString("制作按钮识别失败 (相似度: %1/%2)").arg(makeSimilarity, 0, 'f', 2).arg(makeBrightSimilarity, 0, 'f', 2), LogType::Error);
    }
    
    return recognized;
}

bool StarryCard::verifyRecipeTemplate(const QString& targetRecipe)
{
    if (!hwndGame || !IsWindow(hwndGame)) {
        addLog("游戏窗口无效，无法验证配方模板", LogType::Error);
        return false;
    }
    
    // 截取游戏窗口
    QImage screenshot = captureWindowByHandle(hwndGame, "配方模板验证");
    if (screenshot.isNull()) {
        addLog("截取游戏窗口失败", LogType::Error);
        return false;
    }
    
    // 截取验证区域：(268,344,38,24) - 这就是我们要验证的ROI区域
    QRect verifyROIArea(268, 344, 38, 24);
    QImage verifyROI = screenshot.copy(verifyROIArea);
    
    if (verifyROI.isNull()) {
        addLog("截取配方验证ROI区域失败", LogType::Error);
        return false;
    }
    
    // 使用配方识别器进行比较
    if (!recipeRecognizer->isRecipeTemplatesLoaded()) {
        addLog("配方模板未加载，无法进行验证", LogType::Error);
        return false;
    }
    
    // 获取目标配方模板并提取相同的ROI区域
    const QMap<QString, QImage>& templateImages = recipeRecognizer->getRecipeTemplateImages();
    if (!templateImages.contains(targetRecipe)) {
        addLog(QString("目标配方 '%1' 的模板不存在").arg(targetRecipe), LogType::Error);
        return false;
    }
    
    QImage targetTemplate = templateImages[targetRecipe];
    
    // 从模板中提取ROI区域 (4,4,38,24)
    const QRect RECIPE_ROI(4, 4, 38, 24); // 与RecipeRecognizer中定义相同的ROI
    QImage templateROI = targetTemplate.copy(RECIPE_ROI);
    
    if (templateROI.isNull()) {
        addLog("截取模板ROI区域失败", LogType::Error);
        return false;
    }
    
    // 计算ROI区域的哈希值并比较
    QString verifyHash = calculateImageHash(verifyROI);
    QString templateHash = calculateImageHash(templateROI);
    double similarity = calculateHashSimilarity(verifyHash, templateHash);
    
    // 检查相似度是否大于0.8
    bool verified = (similarity > 0.8);
    
    if (verified) {
        addLog("配方模板验证成功", LogType::Success);
    } else {
        addLog(QString("配方模板验证失败 (相似度: %1)").arg(similarity, 0, 'f', 2), LogType::Error);
    }
    
    return verified;
}

void StarryCard::performCardMaking()
{
    addLog("开始执行制卡流程", LogType::Info);
    
    // 步骤1: 检查游戏窗口
    if (!hwndGame || !IsWindow(hwndGame)) {
        addLog("游戏窗口无效，制卡流程终止", LogType::Error);
        return;
    }
    
    // 步骤1.5: 确保DPI和RecipeRecognizer参数正确设置（修复缩放问题）
    addLog("初始化制卡参数...", LogType::Info);
    if (!recipeRecognizer->isRecipeTemplatesLoaded()) {
        if (!recipeRecognizer->loadRecipeTemplates()) {
            addLog("配方模板加载失败，制卡流程终止", LogType::Error);
            return;
        }
    }
    
    // 设置RecipeRecognizer的参数以确保缩放正确
    recipeRecognizer->setDPI(DPI);
    recipeRecognizer->setGameWindow(hwndGame);
    addLog(QString("已设置制卡参数 - DPI: %1, 游戏窗口句柄: %2").arg(DPI).arg(reinterpret_cast<quintptr>(hwndGame)), LogType::Info);
    
    // 步骤2: 获取选择的配方
    QString targetRecipe;
    if (recipeCombo && recipeCombo->isEnabled() && recipeCombo->currentText() != "无可用配方") {
        targetRecipe = recipeCombo->currentText();
        addLog(QString("选择的配方类型: %1").arg(targetRecipe), LogType::Info);
    } else {
        addLog("未选择配方类型，制卡流程终止", LogType::Error);
        return;
    }
    
    // 步骤3: 配方识别和点击
    addLog("开始配方识别...", LogType::Info);
    recipeRecognizer->recognizeRecipeWithPaging(captureWindowByHandle(hwndGame, "制卡配方识别"), targetRecipe, hwndGame);
    sleepByQElapsedTimer(200); // 等待时间
    
    // 步骤4: 验证配方模板匹配
    addLog("验证配方模板匹配...", LogType::Info);
    
    // 等待一段时间确保页面稳定，然后再验证
    sleepByQElapsedTimer(200); // 等待时间
    
    if (!verifyRecipeTemplate(targetRecipe)) {
        addLog("配方模板验证失败，制卡流程终止", LogType::Error);
        return;
    }
    
    // 步骤5: 识别制作按钮
    addLog("识别制作按钮...", LogType::Info);
    if (!recognizeMakeButton()) {
        addLog("制作按钮识别失败，制卡流程终止", LogType::Error);
        return;
    }
    
    // 步骤6: 使用智能香料分配算法
    const int totalClicks = 12;
    QList<QPair<QString, int>> spiceAllocation = calculateSpiceAllocation(totalClicks);
    
    if (spiceAllocation.isEmpty()) {
        addLog("智能香料分配失败，将直接制作无香料卡片", LogType::Warning);
        spiceAllocation.append(qMakePair("无香料", totalClicks));
    }
    
    // 步骤7: 执行制卡流程
    for (int i = 0; i < spiceAllocation.size(); ++i) {
        const QString& spiceType = spiceAllocation[i].first;
        int currentSpiceClicks = spiceAllocation[i].second;
        
        if (spiceType == "无香料") {
            // 无香料直接点击制作按钮
            addLog(QString("制作无香料卡片，点击制作按钮 %1 次").arg(currentSpiceClicks), LogType::Info);
            for (int click = 0; click < currentSpiceClicks; ++click) {
                leftClickDPI(hwndGame, 285, 425);
                sleepByQElapsedTimer(500); // 改成500ms间隔
                addLog(QString("第 %1 次制作点击完成").arg(click + 1), LogType::Info);
            }
        } else {
            // 选择香料并点击制作
            addLog(QString("选择香料: %1，点击制作按钮 %2 次").arg(spiceType).arg(currentSpiceClicks), LogType::Info);
            
            // 从香料配置获取绑定状态设置
            QPair<bool, bool> bindingConfig = getSpiceBindingConfig(spiceType);
            bool spice_bound = bindingConfig.first;
            bool spice_unbound = bindingConfig.second;
            
            // 识别并点击香料
            QPair<bool, bool> spiceResult = recognizeSpice(spiceType, spice_bound, spice_unbound);
            if (!spiceResult.first) {
                addLog(QString("香料 '%1' 识别失败，跳过").arg(spiceType), LogType::Warning);
                continue;
            }
            
            sleepByQElapsedTimer(500); // 等待香料选择完成
            
            // 点击制作按钮
            for (int click = 0; click < currentSpiceClicks; ++click) {
                leftClickDPI(hwndGame, 285, 425);
                sleepByQElapsedTimer(500); // 改成500ms间隔
                addLog(QString("使用 %1 第 %2 次制作点击完成").arg(spiceType).arg(click + 1), LogType::Info);
            }
            
            // 重新选择下一种香料前等待
            if (i < spiceAllocation.size() - 1) {
                sleepByQElapsedTimer(100);
            }
        }
    }
    
    addLog("制卡流程执行完成", LogType::Success);
}
