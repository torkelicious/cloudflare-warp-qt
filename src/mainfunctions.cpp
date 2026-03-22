#include "mainfunctions.h"
#include <QDebug>
#include <QFile>
#include <QFutureWatcher>
#include <QMap>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QThreadPool>

namespace {
  MainFunctions::CommandResult
  runCommandResultInternal(const QString &program, const QStringList &arguments,
                           int timeoutMs) {
    // keep this on stack because QtConcurrent runs in worker threads.
    QProcess process;
    process.start(program, arguments);

    MainFunctions::CommandResult res;
    if (!process.waitForFinished(timeoutMs)) {
      res.timedOut = true;
      res.exitCode = -1;
      res.err = QStringLiteral("Command timed out");
      process.kill();
      process.waitForFinished(1000);
      return res;
    }

    res.exitCode = process.exitCode();

    res.out = QString::fromUtf8(process.readAllStandardOutput().trimmed());
    res.err = QString::fromUtf8(process.readAllStandardError().trimmed());
    return res;
  }

  const QMap<QString, QString> kModeOutputNormalized = {
    {QStringLiteral("Warp"), QStringLiteral("warp")},
    {QStringLiteral("DnsOverHttps"), QStringLiteral("doh")},
    {QStringLiteral("WarpWithDnsOverHttps"), QStringLiteral("warp+doh")},
    {QStringLiteral("DnsOverTls"), QStringLiteral("dot")},
    {QStringLiteral("WarpWithDnsOverTls"), QStringLiteral("warp+dot")},
    {QStringLiteral("WarpProxy"), QStringLiteral("proxy")},
    {QStringLiteral("TunnelOnly"), QStringLiteral("tunnel_only")},
    {
      QStringLiteral("PostureOnly"),
      QStringLiteral("Device Information Only?")
    }
  };
} // namespace

MainFunctions::MainFunctions(QObject *parent)
  : QObject(parent), resolvWatcher(new QFileSystemWatcher(this)) {
  resolvWatcher->addPath(QStringLiteral("/etc/resolv.conf"));
  connect(resolvWatcher, &QFileSystemWatcher::fileChanged, this, &MainFunctions::checkResolvConf);
  checkResolvConf();
  QThreadPool::globalInstance()->start([this]() {
    refreshCachedMode();
  });
}

void MainFunctions::checkResolvConf() {
  QFile file(QStringLiteral("/etc/resolv.conf"));
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    resolvConnected = false;
    return;
  }
  const QString content = QString::fromUtf8(file.readAll());
  file.close();
  resolvConnected = content.contains(QStringLiteral("127.0.0.2")) ||
                    content.contains(QStringLiteral("127.0.2.2"));
}

QString MainFunctions::runCommand(const QString &program,
                                  const QStringList &arguments) {
  auto res = runCommandResultInternal(program, arguments, 3000);
  if (res.timedOut) {
    return QString();
  }
  return res.out;
}

MainFunctions::CommandResult
MainFunctions::runCommandResult(const QString &program,
                                const QStringList &arguments, int timeoutMs) {
  return runCommandResultInternal(program, arguments, timeoutMs);
}

QFuture<MainFunctions::CommandResult>
MainFunctions::runCommandAsync(const QString &program,
                               const QStringList &arguments, int timeoutMs) {
  return QtConcurrent::run(
    [programCopy = program, argsCopy = arguments, timeoutMs]() {
      return runCommandResultInternal(programCopy, argsCopy, timeoutMs);
    });
}

void MainFunctions::setupToggleOperation(QFuture<CommandResult> future,
                                         bool isConnect) {
  QPointer<MainFunctions> safePtr(this);
  auto watcher = new QFutureWatcher<CommandResult>(this);
  connect(
    watcher, &QFutureWatcher<CommandResult>::finished, watcher,
    [safePtr, watcher, isConnect]() {
      const CommandResult res = watcher->result();
      watcher->deleteLater();

      if (!safePtr)
        return;

      if (isConnect) {
        safePtr->isConnecting = false;
      } else {
        safePtr->isDisconnecting = false;
      }

      if (res.timedOut || res.exitCode != 0 ||
          !res.out.contains(QStringLiteral("Success"), Qt::CaseInsensitive)) {
        const QString msg =
            res.err.isEmpty()
              ? (isConnect
                   ? QStringLiteral("Connection failed or timed out.")
                   : QStringLiteral("Disconnection failed or timed out."))
              : res.err;

        const QString title = isConnect
                                ? QStringLiteral("Warp Connect Error")
                                : QStringLiteral("Warp Disconnect Error");

        emit safePtr->errorOccurred(title, msg);
      }
    });
  watcher->setFuture(future);
}

void MainFunctions::cliConnect() {
  if (isConnecting || isDisconnecting)
    return;
  isConnecting = true;
  if (!isServiceActive()) {
    isConnecting = false;
    return;
  }
  setupToggleOperation(cliConnectAsync(), true);
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliConnectAsync() {
  return runCommandAsync(QStringLiteral("warp-cli"),
                         {QStringLiteral("connect")}, 15000);
}

void MainFunctions::cliDisconnect() {
  if (isConnecting || isDisconnecting)
    return;
  isDisconnecting = true;
  if (!isServiceActive()) {
    isDisconnecting = false;
    return;
  }
  setupToggleOperation(cliDisconnectAsync(), false);
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliDisconnectAsync() {
  return runCommandAsync(QStringLiteral("warp-cli"),
                         {QStringLiteral("disconnect")}, 15000);
}

void MainFunctions::cliRegister() {
  QMessageBox::information(
    nullptr, QStringLiteral("Accept Terms of Service"),
    QStringLiteral("Cloudflare WARP requires accepting its Terms of Service "
      "in a terminal once.\n\n"
      "A terminal window will now open. Please complete "
      "registration and close it when finished."));

  const QString command =
      QStringLiteral("warp-cli --accept-tos registration new");

  const QStringList terminals = {
    QStringLiteral("xdg-terminal"),
    QStringLiteral("x-terminal-emulator"),
    QStringLiteral("alacritty"),
    QStringLiteral("kitty"),
    QStringLiteral("gnome-terminal"),
    QStringLiteral("konsole"),
    QStringLiteral("xfce4-terminal"),
    QStringLiteral("xterm")
  };

  for (const QString &term: terminals) {
    if (QStandardPaths::findExecutable(term).isEmpty())
      continue;

    QStringList args;
    const QString bashCommand =
        command + QStringLiteral("; echo; read -p \"Press Enter to close...\"");

    if (term == QStringLiteral("gnome-terminal"))
      args = {
        QStringLiteral("--"), QStringLiteral("bash"),
        QStringLiteral("-c"), bashCommand
      };
    else if (term == QStringLiteral("xdg-terminal"))
      args = {
        QStringLiteral("-x"), QStringLiteral("bash"),
        QStringLiteral("-c"), bashCommand
      };
    else
      args = {
        QStringLiteral("-e"), QStringLiteral("bash"),
        QStringLiteral("-c"), bashCommand
      };

    QProcess::startDetached(term, args);
    return;
  }

  QMessageBox::critical(
    nullptr, QStringLiteral("Terminal Error"),
    QStringLiteral("No supported terminal emulator found."));
}

QString MainFunctions::cliStatus() {
  return runCommand(QStringLiteral("warp-cli"), {QStringLiteral("status")});
}

QFuture<MainFunctions::CommandResult>
MainFunctions::cliStatusAsync(int timeoutMs) {
  return runCommandAsync(QStringLiteral("warp-cli"), {QStringLiteral("status")},
                         timeoutMs);
}

bool MainFunctions::isServiceActive() {
  // ensure can use system bus
  if (!QDBusConnection::systemBus().isConnected()) {
    qWarning() << "Cannot connect to the D-Bus system bus.";
    return false;
  }

  // internal path to the 'warp-svc' service
  QDBusMessage getUnitMsg = QDBusMessage::createMethodCall(
    QStringLiteral("org.freedesktop.systemd1"),
    QStringLiteral("/org/freedesktop/systemd1"),
    QStringLiteral("org.freedesktop.systemd1.Manager"),
    QStringLiteral("GetUnit")
  );
  getUnitMsg << QStringLiteral("warp-svc.service");

  QDBusReply<QDBusObjectPath> unitReply = QDBusConnection::systemBus().call(getUnitMsg);

  // if service doesn't exist or isn't loaded
  if (!unitReply.isValid()) {
    emit errorOccurred(QStringLiteral("Service Error"),
                       QStringLiteral("The 'warp-svc' service is not running or not found.\n\n"
                         "Please enable it by running:\n"
                         "pkexec systemctl start warp-svc"));
    return false;
  }

  //  Query 'ActiveState' property of service
  QDBusMessage getPropMsg = QDBusMessage::createMethodCall(
    QStringLiteral("org.freedesktop.systemd1"),
    unitReply.value().path(),
    QStringLiteral("org.freedesktop.DBus.Properties"),
    QStringLiteral("Get")
  );
  getPropMsg << QStringLiteral("org.freedesktop.systemd1.Unit")
      << QStringLiteral("ActiveState");

  QDBusReply<QDBusVariant> propReply = QDBusConnection::systemBus().call(getPropMsg);

  // check if the state is "active"
  if (propReply.isValid()) {
    QString state = propReply.value().variant().toString();
    if (state == QStringLiteral("active")) {
      return true;
    }
  }

  // exists but is inactive/failed
  emit errorOccurred(QStringLiteral("Service Error"),
                     QStringLiteral("The 'warp-svc' service is not running.\n\n"
                       "Please enable it by running:\n"
                       "pkexec systemctl start warp-svc"));
  return false;
}

QString MainFunctions::GetCurrentMode() {
  CommandResult output = runCommandResult(QStringLiteral("warp-cli"),
                                          {QStringLiteral("settings")});

  static const QRegularExpression re(
    QStringLiteral(R"(Mode:\s*([A-Za-z0-9]+))"));
  QRegularExpressionMatch match = re.match(output.out);
  if (match.hasMatch()) {
    QString outmatch = match.captured(1);
    return kModeOutputNormalized.value(outmatch, QString());
  }
  return QString();
}

void MainFunctions::refreshCachedMode() { cachedMode = GetCurrentMode(); }

bool MainFunctions::isWarpConnected() {
  if (cachedMode == QStringLiteral("doh") ||
      cachedMode == QStringLiteral("dot")) {
    return resolvConnected;
  }
  QNetworkInterface iface =
      QNetworkInterface::interfaceFromName(QStringLiteral("CloudflareWARP"));
  return iface.isValid() && iface.flags().testFlag(QNetworkInterface::IsUp);
}
