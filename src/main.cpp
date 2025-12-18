#include "mainfunctions.h"
#include "systray.h"
#include "widget.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QSettings>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  a.setApplicationName("CloudflareWarpQt");
  a.setOrganizationName("warp-qt");

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

  // --- Logic to Show Window on Start ---
  QSettings settings;
  bool showFromConfig = settings.value("showOnStart", false).toBool();
  bool showFromCLI = parser.isSet(showOption);

  if (showFromConfig || showFromCLI) {
    w.show(); // Show window if requested by settings OR command line
  }

  return a.exec();
}
