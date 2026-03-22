#include "mainfunctions.h"
#include "systray.h"
#include "widget.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QThreadPool>

int main(int argc, char *argv[]) {
    // turn off unnecessary features
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);
    QApplication a(argc, argv);

    QThreadPool::globalInstance()->setExpiryTimeout(3000);

    a.setApplicationName("cloudflare-warp-qt");
    a.setOrganizationName("warp-qt");
    // remember to set ver later when i feel like it...
    a.setQuitOnLastWindowClosed(false);

    const QString user = QString::fromLocal8Bit(qgetenv("USER"));
    const QString lockName =
            QString("warp-qt.%1.lock")
            .arg(user.isEmpty() ? QStringLiteral("default") : user);
    QLockFile lockFile(QDir::temp().absoluteFilePath(lockName));

    if (!lockFile.tryLock(1500)) {
        QMessageBox::warning(
            nullptr, "WarpQt",
            "The application is already running!\nCheck your system tray.");
        return 1;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt6 GUI for Cloudflare Warp");
    parser.addHelpOption();

    QCommandLineOption showOption("show", "Start with the window visible.");
    parser.addOption(showOption);
    parser.process(a);

    MainFunctions mainFuncs;
    SysTray tray(&mainFuncs);
    tray.setupTray();

    QSettings settings;
    bool showFromConfig = settings.value("showOnStart", false).toBool();
    bool showFromCLI = parser.isSet(showOption);

    if (showFromConfig || showFromCLI) {
        tray.ensureWidget()->show();
    }

    return a.exec();
}
