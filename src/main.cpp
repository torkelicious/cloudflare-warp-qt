#include <QApplication>
#include <QLockFile>
#include <QDir>
#include <QMessageBox>
#include "mainfunctions.h"
#include "systray.h"
#include "widget.h"

// TODO: implement config menu for mode / account type / registration
// TODO: config for auto start & autoconnect
// TODO:Accept cli args to show the widget instead of hiding it

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Single Instance Lock
    QLockFile lockFile(QDir::temp().absoluteFilePath("warp-qt.lock"));

    // If returns false, another instance is running.
    if (!lockFile.tryLock(100)) {
        QMessageBox::warning(nullptr,
                             "WarpQt",
                             "The application is already running!\nCheck your system tray.");
        return 1;
    }

    Widget w;

    // tray apps should start hidden...? i think...
    // w.show();

    SysTray tray(&w);

    QObject::connect(&tray, &SysTray::connectionChanged, &w, &Widget::onConnectionChanged);
    QObject::connect(&w, &Widget::connectionChanged, &tray, &SysTray::updateStatus);

    return a.exec();
}
