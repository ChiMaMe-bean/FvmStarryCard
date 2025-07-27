#include <QDebug>
#include <QDir>
#include <QResource>
#include <QPixmap>
#include <QDirIterator>

void debugResources() {
    qDebug() << "=== Qt资源系统调试 ===";
    qDebug() << "当前工作目录:" << QDir::currentPath();
    
    // 列出资源根目录
    qDebug() << "\n=== 资源根目录内容 ===";
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        qDebug() << path;
        if (path.count() < 50) { // 只显示前50个避免输出过多
            continue;
        } else {
            break;
        }
    }
    
    // 测试具体路径
    qDebug() << "\n=== 测试具体路径 ===";
    QStringList testPaths = {
        ":/background/default.png",
        ":/level/1.png"
    };
    
    for (const QString& path : testPaths) {
        QResource res(path);
        qDebug() << "路径:" << path;
        qDebug() << "  资源有效:" << res.isValid();
        qDebug() << "  资源大小:" << res.size();
        
        QPixmap pixmap(path);
        qDebug() << "  Pixmap加载:" << (!pixmap.isNull() ? "成功" : "失败");
    }
} 