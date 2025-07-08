#include "starrycard.h"
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
#include <QThread>
#include <QTime>
#include <QElapsedTimer>
#include <Windows.h>  // 包含 Windows.h 头文件，用于获取屏幕分辨率
#include <cstring>

// RGB颜色分量提取宏定义(rgb为0x00RRGGBB类型)
#define bgrRValue(rgb)      (LOBYTE((rgb)>>16))  // 红色分量
#define bgrGValue(rgb)      (LOBYTE(((WORD)(rgb)) >> 8))  // 绿色分量
#define bgrBValue(rgb)      (LOBYTE(rgb))  // 蓝色分量
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
#include <QScrollBar>
#include <chrono>

// 定义全局变量，用于存储 DPI 信息
int DPI = 96;  // 默认 DPI 值为 96，即 100% 缩  放

StarryCard::StarryCard(QWidget *parent)
    : QMainWindow(parent)
{
    // 设置DPI感知
    DPI = getDPIFromDC();

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
    loadPageTemplates();
    
    // 初始化 RecipeRecognizer
    recipeRecognizer = new RecipeRecognizer();
    
    // 设置RecipeRecognizer回调函数
    recipeRecognizer->setLogCallback([this](const QString& message, LogType type) {
        addLog(message, type);
    });
    recipeRecognizer->setClickCallback([this](HWND hwnd, int x, int y) -> bool {
        return leftClickDPI(hwnd, x, y);
    });
    recipeRecognizer->setCaptureCallback([this]() -> QImage {
        return captureGameWindow();
    });
    
    // 初始化配方识别模板
    recipeRecognizer->loadRecipeTemplates();
    
    // 更新配方选择下拉框
    updateRecipeCombo();
    
    // 测试图像哈希算法
    qDebug() << "=== 测试图像哈希算法 ===";
    QImage testImage(8, 8, QImage::Format_RGB32);
    testImage.fill(Qt::white);
    QString testHash = calculateImageHash(testImage);
    qDebug() << "8x8白色图像哈希值:" << testHash;
    qDebug() << "=== 图像哈希算法测试结束 ===";
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
    debugCombo->addItems({"配方识别", "卡片识别", "四叶草识别", "刷新测试", "全部功能"});
    debugCombo->setCurrentText("配方识别"); // 设置默认选择
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
            image: url(:/items/icons/downArrow.svg);
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
            image: url(:/items/icons/downArrow.svg);
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
    connect(enhancementBtn, &QPushButton::clicked, this, &StarryCard::startEnhancement);
    rightLayout->addWidget(enhancementBtn);

    // 创建定时器
    enhancementTimer = new QTimer(this);
    connect(enhancementTimer, &QTimer::timeout, this, &StarryCard::performEnhancement);

    // 将三个区域添加到水平布局
    topLayout->addWidget(leftWidget, 1);
    topLayout->addWidget(centerStack, 8);
    topLayout->addWidget(rightWidget, 3);

    // 将水平布局添加到主垂直布局
    mainLayout->addLayout(topLayout, 1);
}

StarryCard::~StarryCard()
{
    if (enhancementTimer) {
        enhancementTimer->stop();
    }
    
    // 清理全局像素数据
    if (globalPixelData) {
        delete[] globalPixelData;
        globalPixelData = nullptr;
        globalBitmapWidth = 0;
        globalBitmapHeight = 0;
    }

    delete recipeRecognizer;
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
    if (!hwndGame || !IsWindow(hwndGame)) {
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
        addLog(QString("开始强化流程，强化范围: %1-%2级").arg(getMinEnhancementLevel()).arg(getMaxEnhancementLevel()), LogType::Success);
    } else {
        stopEnhancement();
    }
}

void StarryCard::stopEnhancement()
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

void StarryCard::performEnhancement()
{
    if (!hwndGame || !IsWindow(hwndGame)) {
        stopEnhancement();
        QMessageBox::warning(this, "错误", "目标窗口已失效，强化已停止！");
        addLog("目标窗口已失效，强化已停止！", LogType::Error);
        return;
    }

    // 检查是否达到最高强化等级
    int maxLevel = getMaxEnhancementLevel();
    int minLevel = getMinEnhancementLevel();
    
    // 如果是第一次强化，从最低等级开始
    if (currentEnhancementLevel == 0) {
        currentEnhancementLevel = minLevel - 1; // 设置为比最低等级低1，下面会加1
    }
    
    if (currentEnhancementLevel >= maxLevel) {
        stopEnhancement();
        addLog(QString("已达到设置的最高强化等级 %1级，强化自动停止").arg(maxLevel), LogType::Success);
        QMessageBox::information(this, "强化完成", QString("已达到设置的最高强化等级 %1级，强化已停止！").arg(maxLevel));
        return;
    }

    // 更新当前强化等级（这里简化为每次执行加1，实际应用中可能需要更复杂的逻辑）
    currentEnhancementLevel++;

    leftClickDPI(hwndGame, 284, 427);
    addLog(QString("执行强化操作：等级 %1 -> %2 (强化范围: %3-%4级)").arg(currentEnhancementLevel-1).arg(currentEnhancementLevel).arg(minLevel).arg(maxLevel), LogType::Info);
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
        qDebug() << "图片加载失败，请检查路径";
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
    for (QPushButton* btn : buttons) {
        if (btn->text() == "开始调试") {
            btn->setEnabled(hwnd != nullptr);
            break;
        }
    }

    if (hwnd) {
        QString title = WindowUtils::getWindowTitle(hwndHall); // 获取大厅窗口标题
        handleDisplayEdit->setText(QString::number(reinterpret_cast<quintptr>(hwnd), 10)); // 显示游戏窗口句柄
        windowTitleLabel->setText(title);
        addLog(QString("已绑定窗口：%1，句柄：%2").arg(title).arg(QString::number(reinterpret_cast<quintptr>(hwnd), 10)), LogType::Success);
    } else {
        handleDisplayEdit->setText("未获取到句柄");
        windowTitleLabel->setText("");
        addLog("窗口绑定失败", LogType::Error);
    }
}

void StarryCard::addLog(const QString& message, LogType type)
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

int StarryCard::getDPIFromDC()
{
    // int screenWidthBeforeAware = GetSystemMetrics(SM_CXSCREEN);
    // SetProcessDPIAware(); // 设置进程DPI感知
    // int screenWidthAfterAware = GetSystemMetrics(SM_CXSCREEN);
    // return int(96 * double(screenWidthAfterAware) / screenWidthBeforeAware + 0.5); // 获取DPI值
    // 获取DPI缩放因子 - 使用更精确的方法
    HDC hdc = GetDC(GetDesktopWindow());
    
    int dpi = 96;  // 默认DPI
    if (hdc) {
        dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(GetDesktopWindow(), hdc);
    }
    return dpi;
}

QImage StarryCard::captureGameWindow()
{
    if (!hwndGame || !IsWindow(hwndGame)) {
        addLog("无效的窗口句柄", LogType::Error);
        return QImage();
    }

    // 获取窗口位置和大小
    RECT rect;
    if (!GetWindowRect(hwndGame, &rect)) {
        addLog("获取窗口位置失败", LogType::Error);
        return QImage();
    }

    // 输出窗口信息
    addLog(QString("窗口位置：(%1, %2) - (%3, %4)").arg(rect.left).arg(rect.top).arg(rect.right).arg(rect.bottom), LogType::Info);
    
    // 获取窗口DC
    HDC hdcWindow = GetDC(hwndGame);
    if (!hdcWindow) {
        addLog("获取窗口DC失败", LogType::Error);
        return QImage();
    }

    // 创建兼容DC和位图
    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        addLog("创建兼容DC失败", LogType::Error);
        ReleaseDC(hwndGame, hdcWindow);
        return QImage();
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    if (!hBitmap) {
        addLog("创建兼容位图失败", LogType::Error);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwndGame, hdcWindow);
        return QImage();
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemDC, hBitmap);

    // 复制窗口内容到位图
    if (!BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY)) {
        addLog("复制窗口内容失败", LogType::Error);
        SelectObject(hdcMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwndGame, hdcWindow);
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
        ReleaseDC(hwndGame, hdcWindow);
        return QImage();
    }

    // 清理资源
    SelectObject(hdcMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwndGame, hdcWindow);

    addLog(QString("成功截取窗口图像：%1x%2").arg(width).arg(height), LogType::Success);
    return image;
}

QImage StarryCard::captureWindowByHandle(HWND hwnd, const QString& windowName)
{
    if (!hwnd || !IsWindow(hwnd)) {
        addLog(QString("无效的窗口句柄: %1").arg(windowName), LogType::Error);
        return QImage();
    }

    // 获取窗口位置和大小
    RECT rect;
    if (!GetWindowRect(hwnd, &rect)) {
        addLog(QString("获取窗口位置失败: %1").arg(windowName), LogType::Error);
        return QImage();
    }

    // 输出窗口信息
    addLog(QString("%1窗口位置：(%2, %3) - (%4, %5)").arg(windowName).arg(rect.left).arg(rect.top).arg(rect.right).arg(rect.bottom), LogType::Info);
    
    // 获取窗口DC
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) {
        addLog(QString("获取窗口DC失败: %1").arg(windowName), LogType::Error);
        return QImage();
    }

    // 创建兼容DC和位图
    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        addLog(QString("创建兼容DC失败: %1").arg(windowName), LogType::Error);
        ReleaseDC(hwnd, hdcWindow);
        return QImage();
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
    if (!hBitmap) {
        addLog(QString("创建兼容位图失败: %1").arg(windowName), LogType::Error);
        DeleteDC(hdcMemDC);
        ReleaseDC(hwnd, hdcWindow);
        return QImage();
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMemDC, hBitmap);

    // 复制窗口内容到位图
    if (!BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY)) {
        addLog(QString("复制窗口内容失败: %1").arg(windowName), LogType::Error);
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
        addLog(QString("获取位图数据失败: %1").arg(windowName), LogType::Error);
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

    addLog(QString("成功截取%1窗口图像：%2x%3").arg(windowName).arg(width).arg(height), LogType::Success);
    return image;
}

void StarryCard::showRecognitionResults(const std::vector<CardInfo>& results)
{
    QString message = "识别到的卡片：\n";
    for (const auto& result : results) {
        message += QString::fromStdString(result.name) + 
                  QString(" (%1星, %2)\n").arg(result.level)
                  .arg(result.isBound ? "已绑定" : "未绑定");
    }
    
    // QMessageBox::information(this, "识别结果", message);
}

void StarryCard::onCaptureAndRecognize()
{
    if (!hwndGame || !IsWindow(hwndGame)) {
        QMessageBox::warning(this, "错误", "请先绑定有效的游戏窗口！");
        return;
    }

    if (!IsGameWindowVisible(hwndGame)) {
        QMessageBox::warning(this, "错误", "游戏窗口不可见，请先打开游戏窗口！");
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
    
    // 根据调试选择执行相应的功能
    if (!debugCombo) {
        addLog("调试选择下拉框未初始化", LogType::Error);
        return;
    }
    
    QString debugMode = debugCombo->currentText();
    addLog(QString("执行调试功能: %1").arg(debugMode), LogType::Info);
    
    // if (debugMode == "网格线调试" || debugMode == "全部功能") {
        
    // }
    
    if (debugMode == "配方识别" || debugMode == "全部功能") {
        // 执行网格线调试
        addLog("开始网格线调试...", LogType::Info);
        recipeRecognizer->debugGridLines(screenshot);
        // 执行配方识别功能
        addLog("开始配方识别...", LogType::Info);
        
        // 选择要匹配的配方模板（动态获取可用的配方类型）
        QStringList availableRecipes = getAvailableRecipeTypes();
        if (availableRecipes.isEmpty()) {
            addLog("没有可用的配方模板，无法进行识别", LogType::Error);
        } else {
            // 从UI中选择配方类型，必须明确选择
            QString targetRecipe;
            if (recipeCombo && recipeCombo->isEnabled() && recipeCombo->currentText() != "无可用配方") {
                targetRecipe = recipeCombo->currentText();
                addLog(QString("从UI选择配方类型: %1").arg(targetRecipe), LogType::Info);
            } else {
                addLog("UI未选择配方类型，配方识别失败", LogType::Error);
                addLog(QString("可用配方类型: %1").arg(availableRecipes.join(", ")), LogType::Info);
                addLog("请在下拉框中选择要识别的配方类型", LogType::Info);
                return; // 直接返回错误，不执行识别
            }
            
            if (!availableRecipes.contains(targetRecipe)) {
                addLog(QString("选择的配方类型 '%1' 不存在，配方识别失败").arg(targetRecipe), LogType::Error);
                addLog(QString("可用配方类型: %1").arg(availableRecipes.join(", ")), LogType::Info);
                return; // 直接返回错误，不执行识别
            }
            addLog(QString("可用配方类型: %1").arg(availableRecipes.join(", ")), LogType::Info);
            addLog(QString("选择匹配模板: %1").arg(targetRecipe), LogType::Info);
            
            // 执行带翻页功能的配方识别
            recipeRecognizer->recognizeRecipeWithPaging(screenshot, targetRecipe, hwndGame);
        }
    }
    
    if (debugMode == "卡片识别" || debugMode == "全部功能") {
        // 执行卡片识别功能
        addLog("开始识别卡片...", LogType::Info);
        
        // 获取配置文件中需要的卡片类型
        QStringList requiredCardTypes = getRequiredCardTypesFromConfig();
        
        std::vector<CardInfo> results;
        if (!requiredCardTypes.isEmpty()) {
            // 使用针对性识别，只识别配置中需要的卡片类型
            results = cardRecognizer->recognizeCardsDetailed(screenshot, requiredCardTypes);
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

    if (debugMode == "刷新测试" || debugMode == "全部功能") {
        // 执行刷新测试功能
        addLog("开始刷新测试...", LogType::Info);
        refreshGameWindow();
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
    // 自动保存配置
    saveEnhancementConfig();
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
    
    // 更新表格显示，禁用低于最低等级和超过最高等级的行
    if (enhancementTable) {
        for (int row = 0; row < 14; ++row) {
            bool enabled = (row + 1) >= minLevel && (row + 1) <= maxLevel;
            
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
    
    // 更新表格显示，禁用低于最低等级和超过最高等级的行
    if (enhancementTable) {
        for (int row = 0; row < 14; ++row) {
            bool enabled = (row + 1) >= minLevel && (row + 1) <= maxLevel;
            
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
    
    addLog(QString("最低强化等级已设置为: %1级").arg(minLevel), LogType::Info);
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
        }
    }
    
    // 加载最低强化等级设置
    if (minEnhancementLevelSpinBox && config.contains("min_enhancement_level")) {
        int minLevel = config["min_enhancement_level"].toInt();
        if (minLevel >= 1 && minLevel <= 14) {
            minEnhancementLevelSpinBox->setValue(minLevel);
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
        }
    }
    
    // 加载最低强化等级设置
    if (minEnhancementLevelSpinBox && config.contains("min_enhancement_level")) {
        int minLevel = config["min_enhancement_level"].toInt();
        if (minLevel >= 1 && minLevel <= 14) {
            minEnhancementLevelSpinBox->setValue(minLevel);
        }
    }
    
    // 触发槽函数以更新表格显示状态
    if (maxEnhancementLevelSpinBox) {
        onMaxEnhancementLevelChanged();
    }
    
    addLog("从文件加载配置成功: " + QFileInfo(fileName).fileName(), LogType::Success);
    
    // 自动保存当前配置以覆盖默认配置
    saveEnhancementConfig();
}

QStringList StarryCard::getCardTypes() const
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

bool StarryCard::loadBindStateTemplate()
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
    addLog("正在翻页到顶部...", LogType::Info);
    
    // 循环点击上翻按钮直到翻到顶部
    int maxPageUpAttempts = 20;
    for (int attempt = 0; attempt < maxPageUpAttempts; ++attempt) {
        leftClickDPI(hwndGame, 532, 539);
        QThread::msleep(100);
        
        if (isPageAtTop()) {
            qDebug() << "成功翻页到顶部，总共点击" << (attempt + 1) << "次";
            addLog(QString("成功翻页到顶部，总共点击 %1 次").arg(attempt + 1), LogType::Success);
            break;
        }
        
        if (attempt == maxPageUpAttempts - 1) {
            qDebug() << "翻页到顶部失败，已达最大尝试次数";
            addLog("翻页到顶部失败", LogType::Warning);
        }
    }
    
    // 步骤2: 先识别当前页的10个四叶草
    qDebug() << "开始识别当前页的四叶草";
    addLog("开始识别当前页的四叶草", LogType::Info);
    
    QImage screenshot = captureGameWindow();
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
                
                qDebug() << "第" << (i + 1) << "个四叶草识别";
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
    addLog("开始逐页向下翻页识别", LogType::Info);
    
    int maxPageDownAttempts = 50;
    for (int pageIndex = 0; pageIndex < maxPageDownAttempts; ++pageIndex) {
        qDebug() << "翻页" << (pageIndex + 1) << "次后检查第十个位置";
        
        // 点击下翻按钮
        leftClickDPI(hwndGame, 535, 563);
        QThread::msleep(100);
        
        // 只检查第十个位置（翻页后这个位置会更新）
        QImage screenshotAfterPage = captureGameWindow();
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
            addLog("已翻页到底部，未找到匹配的四叶草", LogType::Warning);
            break;
        }
        else{
            addLog(QString("第 %1 次翻页后检查完成，继续下一页").arg(pageIndex + 1), LogType::Info);
        }
    }
    
    // 识别失败
    addLog(QString("四叶草识别失败: %1").arg(cloverType), LogType::Error);
    return qMakePair(false, false);
}

bool StarryCard::loadPageTemplates()
{
    // 加载翻页到顶部模板
    QString pageUpPath = ":/items/position/PageUp.png";
    pageUpTemplate = QImage(pageUpPath);
    
    if (pageUpTemplate.isNull()) {
        qDebug() << "无法加载翻页到顶部模板，路径:" << pageUpPath;
        addLog("无法加载翻页到顶部模板", LogType::Error);
        return false;
    }
    
    // 计算翻页到顶部模板的颜色直方图
    pageUpHistogram = calculateColorHistogram(pageUpTemplate);
    qDebug() << "翻页到顶部模板加载成功，颜色直方图特征数:" << pageUpHistogram.size();
    
    // 加载翻页到底部模板
    QString pageDownPath = ":/items/position/PageDown.png";
    pageDownTemplate = QImage(pageDownPath);
    
    if (pageDownTemplate.isNull()) {
        qDebug() << "无法加载翻页到底部模板，路径:" << pageDownPath;
        addLog("无法加载翻页到底部模板", LogType::Error);
        return false;
    }
    
    // 计算翻页到底部模板的颜色直方图
    pageDownHistogram = calculateColorHistogram(pageDownTemplate);
    qDebug() << "翻页到底部模板加载成功，颜色直方图特征数:" << pageDownHistogram.size();
    
    pageTemplatesLoaded = true;
    addLog("翻页模板加载成功", LogType::Success);
    
    // 保存模板图像用于调试
    QString debugDir = QCoreApplication::applicationDirPath() + "/debug_clover";
    QDir().mkpath(debugDir);
    
    QString pageUpDebugPath = QString("%1/page_up_template.png").arg(debugDir);
    if (pageUpTemplate.save(pageUpDebugPath)) {
        qDebug() << "翻页到顶部模板已保存:" << pageUpDebugPath;
    }
    
    QString pageDownDebugPath = QString("%1/page_down_template.png").arg(debugDir);
    if (pageDownTemplate.save(pageDownDebugPath)) {
        qDebug() << "翻页到底部模板已保存:" << pageDownDebugPath;
    }
    
    return true;
}

bool StarryCard::isPageAtTop()
{
    if (!pageTemplatesLoaded || !hwndGame || !IsWindow(hwndGame)) {
        return false;
    }
    
    // 截取游戏窗口
    QImage screenshot = captureGameWindow();
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
    
    QString checkImagePath = QString("%1/page_top_check_%2.png").arg(debugDir).arg(topCheckCount);
    if (pageCheckImage.save(checkImagePath)) {
        qDebug() << "翻页顶部检测图像已保存:" << checkImagePath;
    }
    
    // 计算颜色直方图相似度
    QVector<double> currentHistogram = calculateColorHistogram(pageCheckImage);
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
    QImage screenshot = captureGameWindow();
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
    
    QString checkImagePath = QString("%1/page_bottom_check_%2.png").arg(debugDir).arg(bottomCheckCount);
    if (pageCheckImage.save(checkImagePath)) {
        qDebug() << "翻页底部检测图像已保存:" << checkImagePath;
    }
    
    // 计算颜色直方图相似度
    QVector<double> currentHistogram = calculateColorHistogram(pageCheckImage);
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

// ================== 配方识别功能实现 ==================

// loadRecipeTemplates 函数已迁移到 RecipeRecognizer 类

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
    
    addLog(QString("配方选择下拉框已更新，可用配方: %1").arg(availableRecipes.join(", ")), LogType::Info);
}

// 发送鼠标消息,计算DPI缩放
BOOL StarryCard::leftClickDPI(HWND hwnd, int x, int y)
{
    // 获取DPI缩放因子 - 使用更精确的方法
    // HDC hdc = GetDC(hwnd);  // 获取目标窗口的设备上下文
    // if (!hdc) {
    //     // 如果获取失败，使用桌面的设备上下文
    //     hdc = GetDC(GetDesktopWindow());
    // }
    
    // int dpi = 96;  // 默认DPI
    // if (hdc) {
    //     dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    //     ReleaseDC(hwnd, hdc);
    // }
    
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
        
    case 4: // 纯白色检验 - 接近白色的颜色
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

//在大厅窗口中等待最近服务器出现，返回所属平台
int StarryCard::waitServerInWindow(int *px, int *py)
{
    // 首先检查大厅窗口是否有效
    if (!hwndHall || !IsWindow(hwndHall)) {
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
        if (getActiveGameWindow(hwndHall)) {
            addLog("发现游戏窗口，识别为微端，无需选服", LogType::Success);
            return 0;
        }

        // 依次查找4399、QQ空间、QQ大厅色块，找到直接返回
        for (int platformType = 1; platformType <= 3; platformType++) {
            int result = findLatestServer(platformType, px, py);
            if (result == platformType) {
                addLog(QString("找到选服窗口：平台类型%1").arg(platformType), LogType::Success);
                return platformType;
            }
            // 如果findLatestServer返回-1，说明位图获取失败，提前退出
            if (result == -1) {
                addLog("位图获取失败，停止识别", LogType::Error);
                return -1;
            }
        }
        
                 // 检查纯白色登录界面
         int whiteResult = findLatestServer(4, px, py);
         if (whiteResult == 4) {
             addLog("检测到纯白色登录界面", LogType::Info);
             return 4;
         }
         if (whiteResult == -1) {
             addLog("位图获取失败，停止识别", LogType::Error);
             return -1;
         }
         
         // 检查断网界面
         int disconnectResult = findLatestServer(5, px, py);
         if (disconnectResult == 5) {
             addLog("检测到断网界面", LogType::Warning);
             return 5;
         }
         if (disconnectResult == -1) {
             addLog("位图获取失败，停止识别", LogType::Error);
             return -1;
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
                // addLog(QString("找到平台类型%1的色块：位置(%2,%3)，尺寸(%4x%5)")
                //        .arg(platformType).arg(x).arg(y).arg(width).arg(height), LogType::Success);
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

void StarryCard::refreshGameWindow()
{
    HWND hwndOrigin = getActiveGameWindow(hwndHall);
    addLog(QString("刷新前游戏窗口：%1").arg(QString::number(reinterpret_cast<quintptr>(hwndOrigin), 10)), LogType::Success);

    // 点击刷新按钮
    clickRefresh();

    int counter = 0;
    while(hwndOrigin && hwndOrigin == getActiveGameWindow(hwndHall))
    {
        counter++;
        if(counter > 50) // 5秒
        {
            addLog("刷新失败，点击刷新按钮无效", LogType::Error);
            return;
        }

        sleepByQElapsedTimer(200);
    }
    addLog("刷新成功", LogType::Success);

    sleepByQElapsedTimer(1000);

    hwndServer = getActiveServerWindow(hwndHall);
    if (!hwndServer) {
        addLog("未找到选服窗口，可能是微端或窗口已关闭", LogType::Warning);
        // 继续执行后续识别，不需要矩形信息
    } else {
        addLog(QString("找到选服窗口：%1").arg(QString::number(reinterpret_cast<quintptr>(hwndServer), 10)), LogType::Success);
        
        // 验证选服窗口是否有效
        if (!IsWindow(hwndServer)) {
            addLog("选服窗口句柄无效", LogType::Error);
            hwndServer = nullptr;
        }
    }

    int serverWidth = 0, serverHeight = 0;
    RECT rectHall = {0};
    RECT rectServer = {0};
    
    // 安全地获取大厅窗口矩形
    if (!GetWindowRect(hwndHall, &rectHall)) {
        addLog("获取大厅窗口矩形失败", LogType::Error);
    }
    
    // 只有当选服窗口有效时才获取其矩形
    if (hwndServer && IsWindow(hwndServer)) {
        if (GetWindowRect(hwndServer, &rectServer)) {
            serverWidth = rectServer.right - rectServer.left;
            serverHeight = rectServer.bottom - rectServer.top;
            addLog(QString("选服窗口尺寸：%1x%2").arg(serverWidth).arg(serverHeight), LogType::Info);
        } else {
            addLog("获取选服窗口矩形失败", LogType::Warning);
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
            addLog("创建screenshots文件夹失败", LogType::Error);
        } else {
            addLog("创建screenshots文件夹成功", LogType::Info);
        }
    }
    
    // 截取大厅窗口
    if (hwndHall && IsWindow(hwndHall)) {
        QImage hallImage = captureWindowByHandle(hwndHall, "大厅");
        if (!hallImage.isNull()) {
            QString hallFilename = QString("%1/Ahall_window.png").arg(screenshotsDir);
            if (hallImage.save(hallFilename)) {
                addLog(QString("大厅窗口截图保存成功：%1").arg(hallFilename), LogType::Success);
            } else {
                addLog("大厅窗口截图保存失败", LogType::Error);
            }
        }
    }
    
    // 截取选服窗口
    if (hwndServer && IsWindow(hwndServer)) {
        QImage serverImage = captureWindowByHandle(hwndServer, "选服");
        if (!serverImage.isNull()) {
            QString serverFilename = QString("%1/Aserver_window.png").arg(screenshotsDir);
            if (serverImage.save(serverFilename)) {
                addLog(QString("选服窗口截图保存成功：%1").arg(serverFilename), LogType::Success);
            } else {
                addLog("选服窗口截图保存失败", LogType::Error);
            }
        }
    } else {
        addLog("选服窗口无效，跳过截图", LogType::Warning);
    }

    int x = 0;
    int y = 0;
    int platformType = waitServerInWindow(&x, &y);

    //根据平台和大厅判定坐标确定最近服务器在选服窗口中的坐标
    int latestServerX = 0, latestServerY = 0;//最近服务器在选服窗口中的坐标

    if (platformType == -1)
    {
        addLog("无法识别服务器窗口", LogType::Error);
        return;
    }
    else if (platformType == 0)
    {
        addLog("找到微端窗口，无需选服", LogType::Success);
    }
    else if (platformType == 1)
    {
        latestServerX = (x - 285) * DPI / 96 + rectHall.left - rectServer.left;
        latestServerY = (y + 45) * DPI / 96 + rectHall.top - rectServer.top;
        addLog(QString("找到4399的选服窗口，位置(%2,%3)").arg(latestServerX).arg(latestServerY), LogType::Success);
    }
    else if (platformType == 2)
    {
        latestServerX = (rectServer.right - rectServer.left) / 2 - 115 * DPI / 96; // 中央位置左偏115
        latestServerY = 267 * DPI / 96;
        addLog(QString("找到QQ空间的选服窗口，位置(%2,%3)").arg(x).arg(y), LogType::Success);
    }
    else if (platformType == 3)
    {
        latestServerX = (rectServer.right - rectServer.left) / 2 + 30 * DPI / 96; // 中央位置右偏30
        latestServerY = 580 * DPI / 96;
        addLog(QString("找到QQ大厅的选服窗口，位置(%2,%3)").arg(x).arg(y), LogType::Success);
    }
    else
    {
        addLog("未知的平台类型", LogType::Warning);
        return;
    }

    hwndGame = nullptr; // 刷新后重新获取游戏窗口
    counter = 0;
    while(hwndGame == nullptr) //每1000ms获取1次游戏窗口
    {
        counter++;
        if (counter > 10)
        {
            qDebug() << "无法进入服务器";
            break;
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
            qDebug() << "找到游戏窗口:" << hwndGame;
            break;
        }
    }
    updateHandleDisplay(hwndGame);
}
#include "starrycard.moc"
