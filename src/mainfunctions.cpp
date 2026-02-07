#include "mainfunctions.h"
#include <QProcess>
#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPointer>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>
#include <QMap>

namespace
{
    MainFunctions::CommandResult runCommandResultInternal(const QString &program,
                                                          const QStringList &arguments,
                                                          int timeoutMs)
    {
        QProcess process;
        process.start(program, arguments);

        MainFunctions::CommandResult res;
        if (!process.waitForFinished(timeoutMs))
        {
            res.timedOut = true;
            res.exitCode = -1;
            res.err = QStringLiteral("Command timed out");
            process.kill();
            process.waitForFinished(1000);
            return res;
        }

        res.exitCode = process.exitCode();
        res.out = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        res.err = QString::fromUtf8(process.readAllStandardError()).trimmed();
        return res;
    }
} // namespace

MainFunctions::MainFunctions(QObject *parent) : QObject(parent)
{
    refreshCachedMode();
}

QString MainFunctions::runCommand(const QString &program, const QStringList &arguments)
{
    auto res = runCommandResultInternal(program, arguments, 3000);
    if (res.timedOut)
    {
        return QString();
    }
    return res.out;
}

MainFunctions::CommandResult MainFunctions::runCommandResult(const QString &program,
                                                             const QStringList &arguments,
                                                             int timeoutMs)
{
    return runCommandResultInternal(program, arguments, timeoutMs);
}

QFuture<MainFunctions::CommandResult> MainFunctions::runCommandAsync(const QString &program,
                                                                     const QStringList &arguments,
                                                                     int timeoutMs)
{
    return QtConcurrent::run([programCopy = program, argsCopy = arguments, timeoutMs]()
                             { return runCommandResultInternal(programCopy, argsCopy, timeoutMs); });
}

void MainFunctions::setupToggleOperation(QFuture<CommandResult> future, bool isConnect)
{
    QPointer<MainFunctions> safePtr(this);
    auto watcher = new QFutureWatcher<CommandResult>(this);
    connect(watcher, &QFutureWatcher<CommandResult>::finished, watcher, [safePtr, watcher, isConnect]()
            {
        const CommandResult res = watcher->result();
        watcher->deleteLater();
        if (!safePtr) return;

        if (isConnect) {
            safePtr->isConnecting = false;
        } else {
            safePtr->isDisconnecting = false;
        }

        if (res.timedOut || res.exitCode != 0 || !res.out.contains("Success", Qt::CaseInsensitive)) {
            const QString msg = res.err.isEmpty()
                ? (isConnect ? QStringLiteral("Connection failed or timed out.")
                             : QStringLiteral("Disconnection failed or timed out."))
                : res.err;
            const QString title = isConnect ? QStringLiteral("Warp Connect Error")
                                            : QStringLiteral("Warp Disconnect Error");
            emit safePtr->errorOccurred(title, msg);
        } });
    watcher->setFuture(future);
}

void MainFunctions::cliConnect()
{
    if (isConnecting || isDisconnecting)
        return;
    isConnecting = true;
    if (!isServiceActive())
    {
        isConnecting = false;
        return;
    }
    setupToggleOperation(cliConnectAsync(), true);
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliConnectAsync()
{
    return runCommandAsync("warp-cli", {"connect"}, 15000);
}

void MainFunctions::cliDisconnect()
{
    if (isConnecting || isDisconnecting)
        return;
    isDisconnecting = true;
    if (!isServiceActive())
    {
        isDisconnecting = false;
        return;
    }
    setupToggleOperation(cliDisconnectAsync(), false);
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliDisconnectAsync()
{
    return runCommandAsync("warp-cli", {"disconnect"}, 15000);
}

void MainFunctions::cliRegister()
{
    QMessageBox::information(
        nullptr,
        QStringLiteral("Accept Terms of Service"),
        QStringLiteral("Cloudflare WARP requires accepting its Terms of Service in a terminal once.\n\n"
                       "A terminal window will now open. Please complete registration and close it when finished."));

    const QString command = "warp-cli --accept-tos registration new";
    const QStringList terminals = {
        "x-terminal-emulator",
        "alacritty",
        "kitty",
        "gnome-terminal",
        "konsole",
        "xfce4-terminal",
        "xterm"};

    for (const QString &term : terminals)
    {
        if (QStandardPaths::findExecutable(term).isEmpty())
            continue;

        QStringList args;
        const QString bashCommand = command + "; echo; read -p \"Press Enter to close...\"";

        if (term == "gnome-terminal")
            args = {"--", "bash", "-c", bashCommand};
        else
            args = {"-e", "bash", "-c", bashCommand};

        QProcess::startDetached(term, args);
        return;
    }

    QMessageBox::critical(
        nullptr,
        QStringLiteral("Terminal Error"),
        QStringLiteral("No supported terminal emulator found."));
}

QString MainFunctions::cliStatus()
{
    return runCommand("warp-cli", {"status"});
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliStatusAsync(int timeoutMs)
{
    return runCommandAsync("warp-cli", {"status"}, timeoutMs);
}

bool MainFunctions::isServiceActive()
{
    QProcess process;
    process.start("systemctl", {"is-active", "--quiet", "warp-svc"});
    if (!process.waitForFinished(3000))
    {
        process.kill();
        process.waitForFinished(500);
        return false;
    }

    if (process.exitCode() == 0)
    {
        return true;
    }

    emit errorOccurred("Service Error",
                       "The 'warp-svc' service is not running.\n\n"
                       "Please enable it by running:\n"
                       "pkexec systemctl start warp-svc");
    return false;
}

namespace
{
    const QMap<QString, QString> kModeOutputNormalized = {
        {"Warp", "warp"},
        {"DnsOverHttps", "doh"},
        {"WarpWithDnsOverHttps", "warp+doh"},
        {"DnsOverTls", "dot"},
        {"WarpWithDnsOverTls", "warp+dot"},
        {"WarpProxy", "proxy"},
        {"TunnelOnly", "tunnel_only"},
        {"PostureOnly", "Device Information Only?"}};
}

QString MainFunctions::GetCurrentMode()
{
    CommandResult output = runCommandResult("warp-cli", {"settings"});

    QRegularExpression re(R"(Mode:\s*([A-Za-z0-9]+))");
    QRegularExpressionMatch match = re.match(output.out);
    if (match.hasMatch())
    {
        QString outmatch = match.captured(1);
        return kModeOutputNormalized.value(outmatch, QString());
    }
    return QString();
}

void MainFunctions::refreshCachedMode()
{
    cachedMode = GetCurrentMode();
}

bool MainFunctions::isWarpConnected()
{
    // DNS-only modes don't create a CloudflareWARP interface
    // Check resolv.conf for local DNS proxy instead
    if (cachedMode == "doh" || cachedMode == "dot")
    {
        QFile file("/etc/resolv.conf");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        // WARP uses 127.0.0.2 or 127.0.2.2 as local DNS proxy
        return content.contains("127.0.0.2") || content.contains("127.0.2.2");
    }

    // Tunnel modes (warp, warp+doh, warp+dot, tunnel_only)
    // Check for the CloudflareWARP interface
    QProcess process;
    process.start("ip", {"addr", "show", "CloudflareWARP"});
    if (!process.waitForFinished(2000))
    {
        process.kill();
        process.waitForFinished(500);
        return false;
    }
    return (process.exitCode() == 0);
}