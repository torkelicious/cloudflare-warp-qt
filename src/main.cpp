#include "mainfunctions.h"
#include "systray.h"
#include "widget.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>

// TODO: implement config menu for mode / account type / registration
// TODO: config for auto start & autoconnect
// DONE: Accept cli args to show the widget instead of hiding it

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QApplication::setApplicationName("CloudflareWarpQt");

  // Single Instance Lock
  QLockFile lockFile(QDir::temp().absoluteFilePath("warp-qt.lock"));

  // If returns false, another instance is running.
  if (!lockFile.tryLock(100)) {
    QMessageBox::warning(
        nullptr, "WarpQt",
        "The application is already running!\nCheck your system tray.");
    return 1;
  }

  QCommandLineParser parser;
  parser.setApplicationDescription("Qt6 GUI for Cloudflare Warp");
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption showOption("show", "Start with the window visible.");
  parser.addOption(showOption);
  parser.process(a);

  Widget w;

  SysTray tray(&w);

  QObject::connect(&tray, &SysTray::connectionChanged, &w,
                   &Widget::onConnectionChanged);
  QObject::connect(&w, &Widget::connectionChanged, &tray,
                   &SysTray::updateStatus);

  if (parser.isSet(showOption)) {
    w.show();
  }

  return a.exec();
}
