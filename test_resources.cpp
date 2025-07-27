#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QResource>
#include <QPixmap>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "=== Qt资源系统测试 ===";
    qDebug() << "当前工作目录:" << QDir::currentPath();
    qDebug() << "应用程序目录:" << QCoreApplication::applicationDirPath();
    
    // 测试资源是否注册
    qDebug() << "\n=== 测试资源注册 ===";
    QDir resourceRoot(":");
    if (resourceRoot.exists()) {
        qDebug() << "资源根目录存在";
        QStringList entries = resourceRoot.entryList();
        qDebug() << "资源根目录内容:" << entries;
    } else {
        qDebug() << "资源根目录不存在!";
    }
    
    // 测试具体资源文件
    qDebug() << "\n=== 测试具体资源文件 ===";
    QStringList testPaths = {
        ":/resources/images/background/default.png",
        ":/resources/images/icons/icon128.ico",
        ":/resources/images/level/1.png"
    };
    
    for (const QString& path : testPaths) {
        QPixmap pixmap(path);
        if (!pixmap.isNull()) {
            qDebug() << "✅" << path << "- 尺寸:" << pixmap.size();
        } else {
            qDebug() << "❌" << path << "- 加载失败";
            
            // 检查文件是否在资源系统中存在
            QResource resource(path);
            if (resource.isValid()) {
                qDebug() << "   资源存在但无法加载为QPixmap";
            } else {
                qDebug() << "   资源不存在于资源系统中";
            }
        }
    }
    
    return 0;
} 