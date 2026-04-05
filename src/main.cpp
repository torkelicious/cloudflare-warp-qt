#include "mainfunctions.h"
#include "systray.h"
#include "widget.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QSharedMemory>
#include <QThreadPool>

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);
    QApplication a(argc, argv);

    QThreadPool::globalInstance()->setExpiryTimeout(3000);

    QApplication::setApplicationName("cloudflare-warp-qt");
    QApplication::setOrganizationName("warp-qt");
    QApplication::setQuitOnLastWindowClosed(false);

    const QString user = QString::fromLocal8Bit(qgetenv("USER"));
    const QString lockName =
            QString("warp-qt-lock-%1")
            .arg(user.isEmpty() ? QStringLiteral("default") : user);

    QSharedMemory sharedMem(lockName);

    if (sharedMem.attach()) {
        sharedMem.detach();
    }

    if (!sharedMem.create(1)) {
        QMessageBox::warning(
            nullptr, "WarpQt",
            "The application is already running!\nCheck your system tray.");
        return 1;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt6 GUI for Cloudflare WARP");
    parser.addHelpOption();

    const QCommandLineOption showOption("show", "Start with the window visible.");
    parser.addOption(showOption);
    parser.process(a);

    MainFunctions mainFuncs;
    SysTray tray(&mainFuncs);
    tray.setupTray();

    const QSettings settings;
    if (const bool showFromCLI = parser.isSet(showOption);
        settings.value("showOnStart", false).toBool() || showFromCLI) {
        tray.ensureWidget()->show();
    }

    return QApplication::exec();
}