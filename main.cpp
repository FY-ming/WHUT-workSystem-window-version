// main.cpp文件
// 是系统的入口，用于启动系统。
// 初始化并启动SystemWindow
#include "systemwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QSharedMemory>
#include <QProcess>

using namespace std;

int main(int argc, char *argv[])
{
    // 使用唯一标识确认程序
    QSharedMemory singleton("WHUT_FlagSystem_v1.1");

    QApplication a(argc, argv);
    SystemWindow w;

    // 添加错误检查
    if (!singleton.create(1)) {
        QMessageBox::warning(nullptr,
                             "程序已运行",
                             "国旗班执勤系统已经在运行中\n");

        // 尝试激活已存在的窗口
        QProcess process;
        process.startDetached("whut_flag_system.exe", {"--activate"});
        return 1;
    }

    // 确保程序退出时释放共享内存
    QObject::connect(&a, &QApplication::aboutToQuit, [&](){
        singleton.detach();
    });

    // 处理激活参数，当用户尝试重复启动程序时，自动将已经运行的实例窗口提到前台
    if (argc > 1 && QString(argv[1]) == "--activate") {
        w.setWindowState((w.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        w.raise();
        w.activateWindow();
    }

    w.show();
    return a.exec();
}




