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

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setApplicationName("CloudflareWarpQt");
    a.setOrganizationName("warp-qt");
    //a.setApplicationVersion("1.0");
    // I feel this application is too early for versioning, this is here so i can put it later (i.e so i dont forget)
    // while cmake still has a versioning number im too lazy to care atp
    a.setQuitOnLastWindowClosed(false);

    const QString user = QString::fromLocal8Bit(qgetenv("USER"));
    const QString lockName = QString("warp-qt.%1.lock").arg(user.isEmpty() ? QStringLiteral("default") : user);
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
    //parser.addVersionOption();

    QCommandLineOption showOption("show", "Start with the window visible.");
    parser.addOption(showOption);
    parser.process(a);

    Widget w;
    SysTray tray(&w);

    QObject::connect(&tray, &SysTray::connectionChanged, &w,
                     &Widget::onConnectionChanged);
    QObject::connect(&w, &Widget::connectionChanged, &tray,
                     &SysTray::updateStatus);

    QSettings settings;
    bool showFromConfig = settings.value("showOnStart", false).toBool();
    bool showFromCLI = parser.isSet(showOption);

    if (showFromConfig || showFromCLI) {
        w.show();
    }

    return a.exec();
}
