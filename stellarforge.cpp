#include "stellarforge.h"
#include "custombutton.h"
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
#include <QPalette>
#include <QBrush>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMouseEvent>
#include <Windows.h>  // 包含 Windows.h 头文件，用于获取屏幕分辨率
#include <cstring> 
#include <QTimer>
#include <QMessageBox>
#include <QTime>
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
#include <QScrollBar>

// 定义全局变量，用于存储 DPI 信息
int DPI = 96;  // 默认 DPI 值为 96，即 100% 缩  放

StellarForge::StellarForge(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置DPI感知
    DPI = SetDPIAware();

    // 设置窗口属性
    setWindowTitle("星空强卡器");
    setFixedSize(800, 600);
    
    // 设置窗口图标
    setWindowIcon(QIcon(":/items/icons/icon128.ico"));
    
    // 禁用最大化按钮，只保留最小化和关闭按钮
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    // 检查必要的目录和文件
    qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    qDebug() << "Current working directory:" << QDir::currentPath();
    
    QStringList requiredDirs = {
        "items",
        "items/position",
        "items/card",
        "screenshots"
    };

    for (const QString& dir : requiredDirs) {
        QDir checkDir(dir);
        if (!checkDir.exists()) {
            if (!checkDir.mkpath(".")) {
                qDebug() << "Failed to create directory:" << dir;
            } else {
                qDebug() << "Created directory:" << dir;
            }
        } else {
            qDebug() << "Directory exists:" << dir;
        }
    }

    // 读取存储的自定义背景路径
    loadCustomBgPath();

    // 初始设置默认背景
    setBackground(defaultBgPath);

    // 初始化UI
    setupUI();

    cardRecognizer = new CardRecognizer(this);
    
    // 初始化四叶草识别模板
    loadCloverTemplates();
    loadBindStateTemplate();
    
    // 测试图像哈希算法
    qDebug() << "=== 测试图像哈希算法 ===";
    QImage testImage(8, 8, QImage::Format_RGB32);
    testImage.fill(Qt::white);
    QString testHash = calculateImageHash(testImage);
    qDebug() << "8x8白色图像哈希值:" << testHash;
    qDebug() << "=== 图像哈希算法测试结束 ===";
}

void StellarForge::setupUI()
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

    // 添加三个功能按钮
    QToolButton *btn1 = new QToolButton();
    QToolButton *btn2 = new QToolButton();
    QToolButton *btn3 = new QToolButton();

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
    QList<QToolButton*> buttons = {btn1, btn2, btn3};
    QStringList buttonTexts = {"强化日志", "强化方案", "功能 3"};
    for (int idx = 0; idx < buttons.size(); ++idx) {
        QToolButton *btn = buttons[idx];
        if (btn) {
            btn->setCheckable(true);
            btn->setStyleSheet(buttonStyle);
            btn->setIcon(QIcon::fromTheme("document-new"));
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
    
    // 创建第三个页面 - 与页面二风格保持一致
    QWidget *page3 = new QWidget();
    page3->setStyleSheet("background-color: transparent;");
    
    QVBoxLayout* page3Layout = new QVBoxLayout(page3);
    page3Layout->setContentsMargins(5, 5, 5, 5);
    page3Layout->setSpacing(10);
    
    // 添加标题
    QLabel* page3Title = new QLabel("高级功能配置");
    page3Title->setAlignment(Qt::AlignCenter);
    page3Title->setStyleSheet(R"(
        font-size: 16px;
        font-weight: bold; 
        color: #003D7A;
        background-color: rgba(125, 197, 255, 150);
        border-radius: 8px;
        padding: 8px;
    )");
    page3Layout->addWidget(page3Title);
    
    // 创建配置区域
    QWidget* configArea = new QWidget();
    configArea->setStyleSheet(R"(
        QWidget {
            background-color: rgba(255, 255, 255, 160);
            border: 2px solid rgba(102, 204, 255, 200);
            border-radius: 8px;
        }
    )");
    
    QVBoxLayout* configLayout = new QVBoxLayout(configArea);
    configLayout->setContentsMargins(15, 15, 15, 15);
    configLayout->setSpacing(10);
    
    // 添加一些示例配置选项
    QLabel* configLabel = new QLabel("此页面预留用于其他高级功能配置");
    configLabel->setAlignment(Qt::AlignCenter);
    configLabel->setStyleSheet(R"(
        color: #003D7A;
        font-size: 14px;
        padding: 20px;
    )");
    configLayout->addWidget(configLabel);
    
    // 添加一些占位按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    QPushButton* option1Btn = new QPushButton("选项1");
    option1Btn->setFixedSize(120, 35);
    option1Btn->setStyleSheet(R"(
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
    
    QPushButton* option2Btn = new QPushButton("选项2");
    option2Btn->setFixedSize(120, 35);
    option2Btn->setStyleSheet(R"(
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
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(option1Btn);
    buttonLayout->addWidget(option2Btn);
    buttonLayout->addStretch();
    
    configLayout->addLayout(buttonLayout);
    
    page3Layout->addWidget(configArea, 1);
    page3Layout->addStretch();

    centerStack->addWidget(logPage);
    centerStack->addWidget(enhancementPage);
    centerStack->addWidget(page3);

    // 默认选中"强化日志"按钮
    btn1->setChecked(true);
    centerStack->setCurrentIndex(0);

    // 连接按钮组的信号到页面切换
    connect(buttonGroup, &QButtonGroup::buttonClicked, 
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
    connect(trackMouseBtn, &QPushButton::pressed, this, &StellarForge::startMouseTracking);
    connect(trackMouseBtn, &CustomButton::leftButtonReleased, this, &StellarForge::handleButtonRelease);
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

    // 添加截图识别按钮
    captureBtn = new QPushButton("截图识别", rightWidget);
    captureBtn->setFixedSize(100, 30);
    captureBtn->setEnabled(false); // 初始状态禁用
    captureBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(125, 197, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(125, 197, 255, 255);
            border-radius: 8px;
            font-weight: bold;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: rgba(125, 197, 255, 255);
            color: white;
        }
        QPushButton:pressed {
            background-color: rgba(0, 61, 122, 200);
            color: white;
        }
        QPushButton:disabled {
            background-color: rgba(125, 197, 255, 200);
            color: rgba(0, 61, 122, 100);
            border: 2px solid rgba(200, 200, 200, 150);
        }
    )");
    connect(captureBtn, &QPushButton::clicked, this, &StellarForge::onCaptureAndRecognize);
    rightLayout->addWidget(captureBtn);

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
            background-color: rgba(125, 197, 255, 200);
            color: #003D7A;
            border: 2px solid rgba(125, 197, 255, 255);
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
            image: url(:/items/icons/downArrow.svg);
            width: 12px;
            height: 12px;
            margin-right: 1px;
        }
    )");
    connect(themeCombo, &QComboBox::currentTextChanged, this, &StellarForge::changeBackground);
    connect(themeCombo, &QComboBox::currentTextChanged, this, &StellarForge::updateSelectBtnState);
    rightLayout->addWidget(themeCombo);

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
    connect(selectCustomBgBtn, &QPushButton::clicked, this, &StellarForge::selectCustomBackground);
    selectCustomBgBtn->setEnabled(false);
    selectCustomBgBtn->setFixedHeight(40);
    rightLayout->addWidget(selectCustomBgBtn);

    rightLayout->addStretch(1);

    // 添加强化按钮（保持在底部）
    enhancementBtn = new QPushButton("开始强化");
    enhancementBtn->setFixedHeight(40);
    enhancementBtn->setStyleSheet(R"(
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
    connect(enhancementBtn, &QPushButton::clicked, this, &StellarForge::startEnhancement);
    rightLayout->addWidget(enhancementBtn);

    // 创建定时器
    enhancementTimer = new QTimer(this);
    connect(enhancementTimer, &QTimer::timeout, this, &StellarForge::performEnhancement);

    // 将三个区域添加到水平布局
    topLayout->addWidget(leftWidget, 1);
    topLayout->addWidget(centerStack, 8);
    topLayout->addWidget(rightWidget, 3);

    // 将水平布局添加到主垂直布局
    mainLayout->addLayout(topLayout, 1);
}

StellarForge::~StellarForge()
{
    if (enhancementTimer) {
        enhancementTimer->stop();
    }
}

void StellarForge::startMouseTracking() {
    // 确保DPI感知已启用
    SetProcessDPIAware();
    
    isTracking = true;
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    addLog("开始获取窗口句柄...", LogType::Info);
}

void StellarForge::stopMouseTracking() {
    isTracking = false;
    setCursor(Qt::ArrowCursor);
    setMouseTracking(false);

    QPoint globalPos = QCursor::pos();
    HWND hwnd = WindowUtils::getWindowFromPoint(globalPos);
    
    if (!hwnd) {
        targetWindow = nullptr;
        updateHandleDisplay(nullptr);
        
        // 禁用截图识别按钮
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
        targetWindow = nullptr;
        updateHandleDisplay(nullptr);
        
        // 禁用截图识别按钮
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
    targetWindow = hwnd;
    updateHandleDisplay(hwnd);
    
    // 启用截图识别按钮
    if (captureBtn) {
        captureBtn->setEnabled(true);
    }
    
    qDebug() << "Successfully captured game window handle:" << hwnd;
}

void StellarForge::startEnhancement()
{
    if (!targetWindow || !IsWindow(targetWindow)) {
        QMessageBox::warning(this, "错误", "请先绑定有效的游戏窗口！");
        return;
    }

    if (!isEnhancing && enhancementBtn && enhancementTimer) {
        // 预先加载所需的卡片类型
        requiredCardTypesForEnhancement = getRequiredCardTypesFromConfig();
        
        if (requiredCardTypesForEnhancement.isEmpty()) {
            QMessageBox::warning(this, "警告", "配置文件中未找到有效的卡片类型配置！\n请先在卡片设置中配置主卡和副卡类型。");
            addLog("强化流程启动失败：未找到有效的卡片类型配置", LogType::Error);
            return;
        }
        
        addLog(QString("强化流程准备就绪，将识别以下卡片类型: %1").arg(requiredCardTypesForEnhancement.join(", ")), LogType::Info);
        
        isEnhancing = true;
        currentEnhancementLevel = 0; // 重置当前强化等级
        enhancementBtn->setText("停止强化");
        enhancementTimer->start(1000);
        addLog(QString("开始强化流程，最高强化等级设置为: %1级").arg(getMaxEnhancementLevel()), LogType::Success);
    } else {
        stopEnhancement();
    }
}

void StellarForge::stopEnhancement()
{
    if (enhancementBtn && enhancementTimer) {
        isEnhancing = false;
        enhancementBtn->setText("开始强化");
        enhancementTimer->stop();
        
        // 清空缓存的卡片类型
        requiredCardTypesForEnhancement.clear();
        
        // 重置当前强化等级
        currentEnhancementLevel = 0;
        
        addLog("停止强化流程", LogType::Warning);
    }
}

void StellarForge::performEnhancement()
{
    if (!targetWindow || !IsWindow(targetWindow)) {
        stopEnhancement();
        QMessageBox::warning(this, "错误", "目标窗口已失效，强化已停止！");
        addLog("目标窗口已失效，强化已停止！", LogType::Error);
        return;
    }

    // 检查是否达到最高强化等级
    int maxLevel = getMaxEnhancementLevel();
    if (currentEnhancementLevel >= maxLevel) {
        stopEnhancement();
        addLog(QString("已达到设置的最高强化等级 %1级，强化自动停止").arg(maxLevel), LogType::Success);
        QMessageBox::information(this, "强化完成", QString("已达到设置的最高强化等级 %1级，强化已停止！").arg(maxLevel));
        return;
    }

    // 更新当前强化等级（这里简化为每次执行加1，实际应用中可能需要更复杂的逻辑）
    currentEnhancementLevel++;
    
    // 如果需要在强化过程中进行卡片识别，可以使用预加载的卡片类型
    // 这样避免了每次循环都重新读取配置文件，提高了效率
    if (!requiredCardTypesForEnhancement.isEmpty()) {
        // 示例：在需要时进行针对性卡片识别
        // QImage screenshot = captureGameWindow();
        // if (!screenshot.isNull()) {
        //     std::vector<std::string> results = cardRecognizer->recognizeCards(screenshot, requiredCardTypesForEnhancement);
        //     // 处理识别结果...
        // }
    }

    WindowUtils::clickAtPosition(targetWindow, 284, 427);
    addLog(QString("执行强化操作：等级 %1 -> %2 (最高等级: %3)").arg(currentEnhancementLevel-1).arg(currentEnhancementLevel).arg(maxLevel), LogType::Info);
}

void StellarForge::trackMousePosition(QPoint pos) {
    if (isTracking) {
        // qDebug() << "Mouse position:" << pos;
    }
}

void StellarForge::mouseMoveEvent(QMouseEvent *event) {
    if (isTracking) {
        trackMousePosition(event->globalPos());
    }
    QMainWindow::mouseMoveEvent(event);
}

void StellarForge::mouseReleaseEvent(QMouseEvent *event) {
    if (isTracking && event->button() == Qt::LeftButton) {
        stopMouseTracking();
    }
    QMainWindow::mouseReleaseEvent(event);
}

void StellarForge::handleButtonRelease() {
    if (isTracking) {
        stopMouseTracking();
    }
}

void StellarForge::setBackground(const QString &imagePath)
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
        qDebug() << "图片加载失败，请检查路径";
    }
}

void StellarForge::changeBackground(const QString &theme)
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

void StellarForge::selectCustomBackground()
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

void StellarForge::updateSelectBtnState(const QString &theme)
{
    selectCustomBgBtn->setEnabled(theme == "自定义");
}

void StellarForge::saveCustomBgPath()
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

void StellarForge::loadCustomBgPath()
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

void StellarForge::updateHandleDisplay(HWND hwnd) {
    if (!handleDisplayEdit || !windowTitleLabel) {
        return;
    }

    // 找到截图识别按钮并更新其状态
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        if (btn->text() == "截图识别") {
            btn->setEnabled(hwnd != nullptr);
            break;
        }
    }

    if (hwnd) {
        QString title = WindowUtils::getParentWindowTitle(hwnd);
        handleDisplayEdit->setText(QString::number(reinterpret_cast<quintptr>(hwnd), 10));
        windowTitleLabel->setText(title);
        addLog(QString("已绑定窗口：%1").arg(title), LogType::Success);
    } else {
        handleDisplayEdit->setText("未获取到句柄");
        windowTitleLabel->setText("");
        addLog("窗口绑定失败", LogType::Error);
    }
}

void StellarForge::addLog(const QString& message, LogType type)
{
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
                                .arg(colorStyle)
                                .arg(timestamp)
                                .arg(message);
    
    // 添加新日志
    logTextEdit->append(formattedMessage);

    // 检查日志数量是否超过限制（256条）
    QTextDocument* doc = logTextEdit->document();
    QTextBlock block = doc->begin();
    int lineCount = 0;
    
    // 计算当前日志行数
    while (block.isValid()) {
        lineCount++;
        block = block.next();
    }
    
    // 如果超过256行，删除最早的日志
    if (lineCount > 256) {
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.deleteChar(); // 删除额外的换行符
    }
    
    // 滚动到最新日志
    logTextEdit->verticalScrollBar()->setValue(logTextEdit->verticalScrollBar()->maximum());
}

void StellarForge::updateCurrentBgLabel()
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

int StellarForge::SetDPIAware()
{
  int screenWidthBeforeAware = GetSystemMetrics(SM_CXSCREEN);
  SetProcessDPIAware();//设置进程DPI感知
  int screenWidthAfterAware = GetSystemMetrics(SM_CXSCREEN);
  return int(96 * double(screenWidthAfterAware) / screenWidthBeforeAware + 0.5);//获取DPI值
}

QImage StellarForge::captureGameWindow()
{
    if (!targetWindow || !IsWindow(targetWindow)) {
        addLog("无效的窗口句柄", LogType::Error);
        return QImage();
    }

    // 获取窗口位置和大小
    RECT rect;
    if (!GetWindowRect(targetWindow, &rect)) {
        addLog("获取窗口位置失败", LogType::Error);
        return QImage();
    }

    // 输出窗口信息
    addLog(QString("窗口位置：(%1, %2) - (%3, %4)").arg(rect.left).arg(rect.top).arg(rect.right).arg(rect.bottom), LogType::Info);
    
    // 获取窗口DC
    HDC hdcWindow = GetDC(targetWindow);
    if (!hdcWindow) {
        addLog("获取窗口DC失败", LogType::Error);
        return QImage();
    }

    // 创建兼容DC和位图
    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        addLog("创建兼容DC失败", LogType::Error);
        ReleaseDC(targetWindow, hdcWindow);
        return QImage();
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    if (!hBitmap) {
        addLog("创建兼容位图失败", LogType::Error);
        DeleteDC(hdcMemDC);
        ReleaseDC(targetWindow, hdcWindow);
        return QImage();
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemDC, hBitmap);

    // 复制窗口内容到位图
    if (!BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY)) {
        addLog("复制窗口内容失败", LogType::Error);
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(targetWindow, hdcWindow);
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
        addLog("获取位图数据失败", LogType::Error);
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(targetWindow, hdcWindow);
        return QImage();
    }

    // 清理资源
    SelectObject(hdcMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMemDC);
    ReleaseDC(targetWindow, hdcWindow);

    addLog(QString("成功截取窗口图像：%1x%2").arg(width).arg(height), LogType::Success);
    return image;
}

void StellarForge::showRecognitionResults(const std::vector<std::string>& results)
{
    QString message = "识别到的卡片：\n";
    for (const auto& result : results) {
        message += QString::fromStdString(result) + "\n";
    }
    
    // QMessageBox::information(this, "识别结果", message);
}

void StellarForge::onCaptureAndRecognize()
{
    if (!targetWindow || !IsWindow(targetWindow)) {
        QMessageBox::warning(this, "错误", "请先绑定有效的游戏窗口！");
        return;
    }

    QImage screenshot = captureGameWindow();
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

    // 生成带时间戳的文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = QString("%1/screenshot_%2.png").arg(screenshotsDir).arg(timestamp);

    addLog(QString("正在保存截图到：%1").arg(filename), LogType::Info);

    // 保存截图
    if (screenshot.save(filename)) {
        addLog("截图保存成功", LogType::Success);
        
        // 验证文件是否真的被创建
        QFileInfo checkFile(filename);
        if (checkFile.exists() && checkFile.isFile()) {
            addLog(QString("文件大小：%1 字节").arg(checkFile.size()), LogType::Info);
        } else {
            addLog("文件创建验证失败", LogType::Error);
        }
    } else {
        addLog("截图保存失败", LogType::Error);
        QMessageBox::warning(this, "错误", "无法保存截图");
        return;
    }

    // 调试信息：输出图像信息
    addLog(QString("图像尺寸：%1x%2").arg(screenshot.width()).arg(screenshot.height()), LogType::Info);
    addLog(QString("图像格式：%1").arg(screenshot.format()), LogType::Info);
    
    addLog("开始识别卡片...", LogType::Info);
    
    // 获取配置文件中需要的卡片类型
    QStringList requiredCardTypes = getRequiredCardTypesFromConfig();
    
    std::vector<CardInfo> results;
    if (!requiredCardTypes.isEmpty()) {
        // 使用针对性识别，只识别配置中需要的卡片类型
        results = cardRecognizer->recognizeCardsDetailed(screenshot, requiredCardTypes);
        addLog(QString("使用针对性识别，目标卡片类型: %1").arg(requiredCardTypes.join(", ")), LogType::Info);
    } 
    // else {
    //     // 如果配置为空，使用全量识别作为备选
    //     results = cardRecognizer->recognizeCards(screenshot);
    //     addLog("配置文件中无有效卡片类型，使用全量识别", LogType::Warning);
    // }
    
    if (results.empty()) {
        addLog("未识别到任何卡片", LogType::Warning);
    } else {
        addLog(QString("识别到 %1 张卡片").arg(results.size()), LogType::Success);
    }
    
    // showRecognitionResults(results);
    
    // 测试四叶草识别功能
    addLog("开始测试四叶草识别功能...", LogType::Info);
    qDebug() << "=== 开始四叶草识别测试 ===";
    
    // 测试识别1级四叶草（任意绑定状态）
    QPair<bool, bool> result = recognizeClover("蛇草", true, true);
    if (result.first) {
        addLog(QString("四叶草识别测试成功！绑定状态: %1").arg(result.second ? "绑定" : "未绑定"), LogType::Success);
    } else {
        addLog("四叶草识别测试失败", LogType::Warning);
    }
    
    qDebug() << "=== 四叶草识别测试结束 ===";
}

QWidget* StellarForge::createEnhancementConfigPage()
{
    QWidget* page = new QWidget();
    page->setStyleSheet("background-color: transparent;");
    
          QVBoxLayout* layout = new QVBoxLayout(page);
      layout->setContentsMargins(0, 0, 0, 0);  // 减少边距以增加表格可用空间
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
            image: url(:/items/icons/downArrow.svg);
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
                this, &StellarForge::onEnhancementConfigChanged);
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
                this, &StellarForge::onEnhancementConfigChanged);
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
                this, &StellarForge::onEnhancementConfigChanged);
        enhancementTable->setCellWidget(row, 3, subCard3);
        
        // 四叶草下拉框
        QComboBox* clover = new QComboBox();
        QStringList cloverTypes = {"无", "1级", "2级", "3级", "4级", "5级", "6级", "S", "SS", "SSS", "SSR", "蛇草"};
        clover->addItems(cloverTypes);
        clover->setProperty("row", row);
        clover->setProperty("type", "clover");
        connect(clover, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                this, &StellarForge::onEnhancementConfigChanged);
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
        
        connect(bindCheck, &QCheckBox::stateChanged, this, &StellarForge::onEnhancementConfigChanged);
        connect(unboundCheck, &QCheckBox::stateChanged, this, &StellarForge::onEnhancementConfigChanged);
        
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
        connect(cardSettingBtn, &QPushButton::clicked, this, &StellarForge::onCardSettingButtonClicked);
        enhancementTable->setCellWidget(row, 6, cardSettingBtn);
    }
    
    layout->addWidget(enhancementTable);
    
    // 添加最高强化等级设置
    QHBoxLayout* maxLevelLayout = new QHBoxLayout();
    maxLevelLayout->setSpacing(10);
    maxLevelLayout->setContentsMargins(10, 5, 10, 5);
    
    QLabel* maxLevelLabel = new QLabel("最高强化等级:");
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
    maxEnhancementLevelSpinBox->setSuffix(" 级");
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
            this, &StellarForge::onMaxEnhancementLevelChanged);
    
    // QLabel* descLabel = new QLabel("(程序执行强化时将只进行1级到所选最高等级的强化)");
    // descLabel->setStyleSheet(R"(
    //     QLabel {
    //         color: #666666;
    //         font-size: 11px;
    //         font-style: italic;
    //     }
    // )");
    
    maxLevelLayout->addWidget(maxLevelLabel);
    maxLevelLayout->addWidget(maxEnhancementLevelSpinBox);
    // maxLevelLayout->addWidget(descLabel);
    maxLevelLayout->addStretch();
    
    layout->addLayout(maxLevelLayout);
    
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
    connect(loadBtn, &QPushButton::clicked, this, &StellarForge::loadEnhancementConfigFromFile);
    
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
    connect(saveBtn, &QPushButton::clicked, this, &StellarForge::saveEnhancementConfig);
    
    btnLayout->addStretch();
    btnLayout->addWidget(loadBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    // 加载现有配置
    loadEnhancementConfig();
    
    return page;
}

int StellarForge::getMinSubCardLevel(int enhancementLevel)
{
    if (enhancementLevel <= 2) {
        return 0;
    } else if (enhancementLevel == 3) {
        return 1;
    } else {
        return qMax(0, enhancementLevel - 2);
    }
}

int StellarForge::getMaxSubCardLevel(int enhancementLevel)
{
    return enhancementLevel;
}

int StellarForge::getMaxEnhancementLevel() const
{
    if (maxEnhancementLevelSpinBox) {
        return maxEnhancementLevelSpinBox->value();
    }
    return 14; // 默认最高等级为14
}

void StellarForge::onEnhancementConfigChanged()
{
    // 自动保存配置
    saveEnhancementConfig();
}

void StellarForge::onMaxEnhancementLevelChanged()
{
    if (!maxEnhancementLevelSpinBox) return;
    
    int maxLevel = maxEnhancementLevelSpinBox->value();
    
    // 更新表格显示，禁用超过最高等级的行
    if (enhancementTable) {
        for (int row = 0; row < 14; ++row) {
            bool enabled = (row + 1) <= maxLevel;
            
            // 设置行的启用状态
            for (int col = 0; col < enhancementTable->columnCount(); ++col) {
                QWidget* widget = enhancementTable->cellWidget(row, col);
                if (widget) {
                    widget->setEnabled(enabled);
                }
            }
            
            // 设置行的视觉效果
            for (int col = 0; col < enhancementTable->columnCount(); ++col) {
                QWidget* widget = enhancementTable->cellWidget(row, col);
                if (widget) {
                    if (enabled) {
                        widget->setStyleSheet(widget->styleSheet().replace("color: #CCCCCC;", "color: #003D7A;"));
                    } else {
                        QString currentStyle = widget->styleSheet();
                        if (!currentStyle.contains("color: #CCCCCC;")) {
                            currentStyle = currentStyle.replace("color: #003D7A;", "color: #CCCCCC;");
                            widget->setStyleSheet(currentStyle);
                        }
                    }
                }
            }
        }
    }
    
    // 保存配置
    saveEnhancementConfig();
    
    addLog(QString("最高强化等级已设置为: %1级").arg(maxLevel), LogType::Info);
}

void StellarForge::saveEnhancementConfig()
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

void StellarForge::loadEnhancementConfig()
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
    
    for (int row = 0; row < 14; ++row) {
        QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
        if (!config.contains(levelKey)) continue;
        
        QJsonObject levelConfig = config[levelKey].toObject();
        
        // 设置副卡配置
        QComboBox* subCard1 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 1));
        if (subCard1 && levelConfig.contains("subcard1")) {
            int value = levelConfig["subcard1"].toInt();
            int index = value - getMinSubCardLevel(row);
            if (index >= 0 && index < subCard1->count()) {
                subCard1->setCurrentIndex(index);
            }
        }
        
        QComboBox* subCard2 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 2));
        if (subCard2 && levelConfig.contains("subcard2")) {
            int value = levelConfig["subcard2"].toInt();
            if (value == -1) {
                subCard2->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard2->count()) {
                    subCard2->setCurrentIndex(index);
                }
            }
        }
        
        QComboBox* subCard3 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 3));
        if (subCard3 && levelConfig.contains("subcard3")) {
            int value = levelConfig["subcard3"].toInt();
            if (value == -1) {
                subCard3->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard3->count()) {
                    subCard3->setCurrentIndex(index);
                }
            }
        }
        
        // 设置四叶草配置
        QComboBox* clover = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 4));
        if (clover && levelConfig.contains("clover")) {
            QString value = levelConfig["clover"].toString();
            int index = clover->findText(value);
            if (index >= 0) {
                clover->setCurrentIndex(index);
            }
        }
        
        // 设置四叶草状态配置
        QWidget* cloverStateWidget = enhancementTable->cellWidget(row, 5);
        QHBoxLayout* cloverStateLayout = qobject_cast<QHBoxLayout*>(cloverStateWidget->layout());
        QCheckBox* bindCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(0)->widget());
        QCheckBox* unboundCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(1)->widget());
        
        if (bindCheck && unboundCheck) {
            if (levelConfig.contains("clover_bound")) {
                bindCheck->setChecked(levelConfig["clover_bound"].toBool());
            }
            if (levelConfig.contains("clover_unbound")) {
                unboundCheck->setChecked(levelConfig["clover_unbound"].toBool());
            }
        }
        
        // 卡片设置配置现在通过对话框处理，不再需要在这里加载
    }
    
    // 加载最高强化等级设置
    if (maxEnhancementLevelSpinBox && config.contains("max_enhancement_level")) {
        int maxLevel = config["max_enhancement_level"].toInt();
        if (maxLevel >= 1 && maxLevel <= 14) {
            maxEnhancementLevelSpinBox->setValue(maxLevel);
            // 触发槽函数以更新表格显示状态
            onMaxEnhancementLevelChanged();
        }
    }
    
    addLog("强化方案配置已加载", LogType::Success);
}

void StellarForge::loadEnhancementConfigFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
        "选择强化配置文件", 
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
    
    // 加载配置到表格
    for (int row = 0; row < 14; ++row) {
        QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
        if (!config.contains(levelKey)) continue;
        
        QJsonObject levelConfig = config[levelKey].toObject();
        
        // 设置副卡配置
        QComboBox* subCard1 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 1));
        if (subCard1 && levelConfig.contains("subcard1")) {
            int value = levelConfig["subcard1"].toInt();
            int index = value - getMinSubCardLevel(row);
            if (index >= 0 && index < subCard1->count()) {
                subCard1->setCurrentIndex(index);
            }
        }
        
        QComboBox* subCard2 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 2));
        if (subCard2 && levelConfig.contains("subcard2")) {
            int value = levelConfig["subcard2"].toInt();
            if (value == -1) {
                subCard2->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard2->count()) {
                    subCard2->setCurrentIndex(index);
                }
            }
        }
        
        QComboBox* subCard3 = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 3));
        if (subCard3 && levelConfig.contains("subcard3")) {
            int value = levelConfig["subcard3"].toInt();
            if (value == -1) {
                subCard3->setCurrentIndex(0);
            } else {
                int index = value - getMinSubCardLevel(row) + 1;
                if (index >= 0 && index < subCard3->count()) {
                    subCard3->setCurrentIndex(index);
                }
            }
        }
        
        // 设置四叶草配置
        QComboBox* clover = qobject_cast<QComboBox*>(enhancementTable->cellWidget(row, 4));
        if (clover && levelConfig.contains("clover")) {
            QString value = levelConfig["clover"].toString();
            int index = clover->findText(value);
            if (index >= 0) {
                clover->setCurrentIndex(index);
            }
        }
        
        // 设置四叶草状态配置
        QWidget* cloverStateWidget = enhancementTable->cellWidget(row, 5);
        QHBoxLayout* cloverStateLayout = qobject_cast<QHBoxLayout*>(cloverStateWidget->layout());
        QCheckBox* bindCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(0)->widget());
        QCheckBox* unboundCheck = qobject_cast<QCheckBox*>(cloverStateLayout->itemAt(1)->widget());
        
        if (bindCheck && unboundCheck) {
            if (levelConfig.contains("clover_bound")) {
                bindCheck->setChecked(levelConfig["clover_bound"].toBool());
            }
            if (levelConfig.contains("clover_unbound")) {
                unboundCheck->setChecked(levelConfig["clover_unbound"].toBool());
            }
        }
        
        // 卡片设置配置现在通过对话框处理，不再需要在这里加载
    }
    
    // 加载最高强化等级设置
    if (maxEnhancementLevelSpinBox && config.contains("max_enhancement_level")) {
        int maxLevel = config["max_enhancement_level"].toInt();
        if (maxLevel >= 1 && maxLevel <= 14) {
            maxEnhancementLevelSpinBox->setValue(maxLevel);
            // 触发槽函数以更新表格显示状态
            onMaxEnhancementLevelChanged();
        }
    }
    
    addLog("从文件加载配置成功: " + QFileInfo(fileName).fileName(), LogType::Success);
    
    // 自动保存当前配置以覆盖默认配置
    saveEnhancementConfig();
}

QStringList StellarForge::getCardTypes() const
{
    QStringList cardTypes;
    if (cardRecognizer) {
        // 从卡片识别器获取已加载的卡片类型
        auto registeredCards = cardRecognizer->getRegisteredCards();
        for (const auto& card : registeredCards) {
            cardTypes.append(QString::fromStdString(card));
        }
    }
    
    // 如果没有加载到卡片类型，从资源文件直接获取
    if (cardTypes.isEmpty()) {
        QDir resourceDir(":/items/card");
        QStringList nameFilters;
        nameFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
        QStringList cardFiles = resourceDir.entryList(nameFilters, QDir::Files);
        
        for (const QString& cardFile : cardFiles) {
            QString cardName = QFileInfo(cardFile).baseName();
            cardTypes.append(cardName);
        }
        
        // 如果资源文件也没有，提供默认选项
        if (cardTypes.isEmpty()) {
            cardTypes << "煮蛋器投手" << "巧克力面包" << "三线酒架" << "面粉袋" << "香肠";
        }
    }
    
    return cardTypes;
}

QStringList StellarForge::getRequiredCardTypesFromConfig()
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
    
    // 遍历所有等级的配置
    for (int row = 0; row < 14; ++row) {
        QString levelKey = QString("%1-%2").arg(row).arg(row + 1);
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

void StellarForge::onCardSettingButtonClicked()
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
            this, &StellarForge::onApplyMainCardToAll);
    connect(&dialog, &CardSettingDialog::applySubCardToAll,
            this, &StellarForge::onApplySubCardToAll);
    
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

void StellarForge::onApplyMainCardToAll(int row, const QString& cardType, bool bind, bool unbound)
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

void StellarForge::onApplySubCardToAll(int row, const QString& cardType, bool bind, bool unbound)
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

void StellarForge::saveCardSettingForRow(int row, const QString& mainCardType, const QString& subCardType,
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
            border: 2px solid rgba(102, 204, 255, 255);
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
    m_mainCardCombo->addItem("无");
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
    connect(m_mainBindCheck, &QCheckBox::stateChanged, this, &CardSettingDialog::onConfigChanged);
    connect(m_mainUnboundCheck, &QCheckBox::stateChanged, this, &CardSettingDialog::onConfigChanged);
    connect(m_subCardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CardSettingDialog::onConfigChanged);
    connect(m_subBindCheck, &QCheckBox::stateChanged, this, &CardSettingDialog::onConfigChanged);
    connect(m_subUnboundCheck, &QCheckBox::stateChanged, this, &CardSettingDialog::onConfigChanged);
    
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

bool StellarForge::loadCloverTemplates()
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
        QString filePath = QString(":/items/clover/%1").arg(cloverTypes[i]);
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
        
        qDebug() << "成功加载四叶草模板:" << cloverTypes[i] << "颜色直方图特征数:" << histogram.size();
        
        // 保存模板图像和ROI区域用于调试
        QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
        QDir().mkpath(debugDir);
        
        QString templatePath = QString("%1/template_%2.png").arg(debugDir).arg(cloverTypes[i]);
        if (template_image.save(templatePath)) {
            qDebug() << "模板图像已保存:" << templatePath;
        }
        
        QImage templateROI = template_image.copy(roi);
        QString templateROIPath = QString("%1/template_roi_%2.png").arg(debugDir).arg(cloverTypes[i]);
        if (templateROI.save(templateROIPath)) {
            qDebug() << "模板ROI区域已保存:" << templateROIPath;
        }
    }
    
    cloverTemplatesLoaded = !cloverTemplateHistograms.isEmpty();
    if (cloverTemplatesLoaded) {
        qDebug() << "四叶草模板加载完成，总数:" << cloverTemplateHistograms.size();
        addLog(QString("成功加载 %1 个四叶草模板").arg(cloverTemplateHistograms.size()), LogType::Success);
    } else {
        qDebug() << "四叶草模板加载失败，没有成功加载任何模板";
        addLog("四叶草模板加载失败", LogType::Error);
    }
    
    return cloverTemplatesLoaded;
}

bool StellarForge::loadBindStateTemplate()
{
    QString filePath = ":/items/bind_state/clover_bound.png";
    bindStateTemplate = QImage(filePath);
    
    if (bindStateTemplate.isNull()) {
        qDebug() << "无法加载绑定状态模板，路径:" << filePath;
        addLog("无法加载绑定状态模板", LogType::Error);
        return false;
    }
    
    // 计算绑定状态模板的哈希值
    bindStateTemplateHash = calculateImageHash(bindStateTemplate);
    qDebug() << "绑定状态模板加载成功，哈希值:" << bindStateTemplateHash;
    addLog("绑定状态模板加载成功", LogType::Success);
    
    // 保存绑定状态模板用于调试
    QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
    QDir().mkpath(debugDir);
    
    QString bindTemplatePath = QString("%1/bind_state_template.png").arg(debugDir);
    if (bindStateTemplate.save(bindTemplatePath)) {
        qDebug() << "绑定状态模板已保存:" << bindTemplatePath;
    }
    
    return true;
}

QString StellarForge::calculateImageHash(const QImage& image, const QRect& roi)
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

QVector<double> StellarForge::calculateColorHistogram(const QImage& image, const QRect& roi)
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

double StellarForge::calculateColorHistogramSimilarity(const QImage& image1, const QImage& image2, const QRect& roi)
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

bool StellarForge::isCloverBound(const QImage& cloverImage)
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
    qDebug() << "绑定状态对比 - 模板哈希:" << bindStateTemplateHash << "当前哈希:" << currentHash;
    
    // 保存当前绑定状态区域用于调试
    static int bindCheckCount = 0;
    bindCheckCount++;
    QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
    QDir().mkpath(debugDir);
    
    QImage currentBindROI = cloverImage.copy(bindStateROI);
    QString currentBindPath = QString("%1/current_bind_roi_%2.png").arg(debugDir).arg(bindCheckCount);
    if (currentBindROI.save(currentBindPath)) {
        qDebug() << "当前绑定状态ROI区域已保存:" << currentBindPath;
    }
    
    // 比较哈希值，匹配度为1表示完全匹配
    return (currentHash == bindStateTemplateHash);
}

QPair<bool, bool> StellarForge::recognizeClover(const QString& cloverType, bool clover_bound, bool clover_unbound)
{
    if (!cloverTemplatesLoaded) {
        addLog("四叶草模板未加载，无法进行识别", LogType::Error);
        return qMakePair(false, false);
    }
    
    if (!targetWindow || !IsWindow(targetWindow)) {
        addLog("游戏窗口无效，无法进行四叶草识别", LogType::Error);
        return qMakePair(false, false);
    }
    
    addLog(QString("开始识别四叶草: %1").arg(cloverType), LogType::Info);
    
    // 步骤1: 调整区域内四叶草位置 - 点击3次翻页按钮
    qDebug() << "开始调整四叶草位置，点击翻页按钮(532, 539) 3次";
    for (int i = 0; i < 3; ++i) {
        WindowUtils::clickAtPosition(targetWindow, 532, 539);
        QThread::msleep(100);
    }
    addLog("完成翻页按钮点击调整", LogType::Info);
    
    // 循环进行识别，最多尝试两轮（第一轮直接识别，第二轮向下翻页后识别）
    for (int round = 0; round < 2; ++round) {
        if (round > 0) {
            // 步骤4: 向下翻页
            addLog("向下翻页继续识别", LogType::Info);
            for (int i = 0; i < 3; ++i) {
                WindowUtils::clickAtPosition(targetWindow, 535, 563);
                QThread::msleep(100);
            }
        }
        
        // 步骤2: 截取并分析区域
        qDebug() << "第" << (round + 1) << "轮识别，开始截图";
        QImage screenshot = captureGameWindow();
        if (screenshot.isNull()) {
            qDebug() << "截图失败";
            addLog("截图失败", LogType::Error);
            continue;
        }
        qDebug() << "截图成功，尺寸:" << screenshot.size();
        
        // 截取四叶草区域: (33,526)为左上角，宽490，高49
        QRect cloverArea(33, 526, 490, 49);
        QImage cloverStrip = screenshot.copy(cloverArea);
        
        if (cloverStrip.isNull()) {
            qDebug() << "四叶草区域截取失败，区域:" << cloverArea;
            addLog("四叶草区域截取失败", LogType::Error);
            continue;
        }
        qDebug() << "四叶草区域截取成功，尺寸:" << cloverStrip.size();
        
        // 保存四叶草区域图像用于调试
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
        QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
        QDir().mkpath(debugDir);
        
        QString cloverStripPath = QString("%1/clover_strip_%2_round%3.png").arg(debugDir).arg(timestamp).arg(round + 1);
        if (cloverStrip.save(cloverStripPath)) {
            qDebug() << "四叶草区域图像已保存:" << cloverStripPath;
        } else {
            qDebug() << "四叶草区域图像保存失败:" << cloverStripPath;
        }
        
        // 将区域分成10张49x49的图片进行识别
        for (int i = 0; i < 10; ++i) {
            int x_offset = i * 49;
            QRect individualCloverRect(x_offset, 0, 49, 49);
            QImage singleClover = cloverStrip.copy(individualCloverRect);
            
            if (singleClover.isNull()) {
                continue;
            }
            
            // 保存所有四叶草图像用于调试
            QString singleCloverPath = QString("%1/single_clover_%2_round%3_pos%4.png")
                .arg(debugDir).arg(timestamp).arg(round + 1).arg(i + 1);
            if (singleClover.save(singleCloverPath)) {
                if (i == 0) qDebug() << "四叶草图像已保存:" << singleCloverPath << "(总共将保存10个)";
            } else {
                qDebug() << "四叶草图像保存失败:" << singleCloverPath;
            }
            
            // 同时保存所有ROI区域用于对比
            QRect cloverROI(4, 4, 38, 24);
            QImage roiImage = singleClover.copy(cloverROI);
            QString roiPath = QString("%1/clover_roi_%2_round%3_pos%4.png")
                .arg(debugDir).arg(timestamp).arg(round + 1).arg(i + 1);
            if (roiImage.save(roiPath)) {
                if (i == 0) qDebug() << "四叶草ROI区域已保存:" << roiPath << "(总共将保存10个)";
            } else {
                qDebug() << "四叶草ROI区域保存失败:" << roiPath;
            }
            
            // 使用颜色直方图进行匹配
            double similarity = 0.0;
            if (cloverTemplateImages.contains(cloverType)) {
                QImage templateImage = cloverTemplateImages[cloverType];
                similarity = calculateColorHistogramSimilarity(singleClover, templateImage, cloverROI);
            }
            
            // 输出相似度用于调试
            qDebug() << "位置" << (i + 1) << "与" << cloverType << "的相似度:" << QString::number(similarity, 'f', 4);
            
            // 输出与所有模板的相似度（仅第一个位置，避免输出过多）
            if (i == 0) {
                qDebug() << "=== 位置1与所有模板的相似度对比 ===";
                for (auto it = cloverTemplateImages.begin(); it != cloverTemplateImages.end(); ++it) {
                    double allSimilarity = calculateColorHistogramSimilarity(singleClover, it.value(), cloverROI);
                    qDebug() << "与" << it.key() << ":" << QString::number(allSimilarity, 'f', 4);
                }
                qDebug() << "=== 相似度对比结束 ===";
            }
            
            // 设置相似度阈值（可根据实际情况调整）
            double similarityThreshold = 1.00; // 100%相似度
            
            // 与目标四叶草模板比较
            if (similarity >= similarityThreshold) {
                
                addLog(QString("找到匹配的四叶草: %1 (位置: %2)").arg(cloverType).arg(i + 1), LogType::Success);
                
                // 步骤3: 检查绑定状态（如果需要）
                bool needBindCheck = clover_bound || clover_unbound;
                bool actualBindState = false;
                
                if (needBindCheck) {
                    actualBindState = isCloverBound(singleClover);
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
                        continue;
                    }
                }
                
                // 步骤5: 点击四叶草中心位置
                int click_x = 33 + x_offset + 24; // 左边界 + 偏移 + 半宽
                int click_y = 526 + 24; // 上边界 + 半高
                
                WindowUtils::clickAtPosition(targetWindow, click_x, click_y);
                addLog(QString("点击四叶草中心位置: (%1, %2)").arg(click_x).arg(click_y), LogType::Success);
                
                return qMakePair(true, actualBindState);
            }
        }
        
        addLog(QString("第 %1 轮识别未找到匹配的四叶草").arg(round + 1), LogType::Info);
    }
    
    // 识别失败
    addLog(QString("四叶草识别失败: %1").arg(cloverType), LogType::Error);
    return qMakePair(false, false);
}

#include "stellarforge.moc"
