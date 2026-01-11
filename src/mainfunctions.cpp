#include "mainfunctions.h"
#include <QProcess>
#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPointer>
#include <QFile>
#include <map>

namespace {
    MainFunctions::CommandResult runCommandResultInternal(const QString &program,
                                                          const QStringList &arguments,
                                                          int timeoutMs) {
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
        res.out = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        res.err = QString::fromUtf8(process.readAllStandardError()).trimmed();
        return res;
    }
} // namespace

MainFunctions::MainFunctions(QObject *parent) : QObject(parent) {
    refreshCachedMode();
}

QString MainFunctions::runCommand(const QString &program, const QStringList &arguments) {
    auto res = runCommandResultInternal(program, arguments, 3000);
    if (res.timedOut) {
        return QString();
    }
    return res.out;
}

MainFunctions::CommandResult MainFunctions::runCommandResult(const QString &program,
                                                             const QStringList &arguments,
                                                             int timeoutMs) {
    return runCommandResultInternal(program, arguments, timeoutMs);
}

QFuture<MainFunctions::CommandResult> MainFunctions::runCommandAsync(const QString &program,
                                                                     const QStringList &arguments,
                                                                     int timeoutMs) {
    return QtConcurrent::run([programCopy = program, argsCopy = arguments, timeoutMs]() {
        return runCommandResultInternal(programCopy, argsCopy, timeoutMs);
    });
}

void MainFunctions::cliConnect() {
    if (isConnecting || isDisconnecting)
        return; // Prevent concurrent operations
    isConnecting = true;
    if (!isServiceActive()) {
        isConnecting = false;
        return;
    }
    QFuture<CommandResult> future = cliConnectAsync();
    QPointer < MainFunctions > self(this);
    auto watcher = new QFutureWatcher<CommandResult>(this);
    connect(watcher, &QFutureWatcher<CommandResult>::finished, watcher, [self, watcher]() {
        const CommandResult res = watcher->result();
        watcher->deleteLater();
        if (!self) return;

        self->isConnecting = false;
        if (res.timedOut || res.exitCode != 0 || !res.out.contains("Success", Qt::CaseInsensitive)) {
            const QString msg = res.err.isEmpty() ? QStringLiteral("Connection failed or timed out.") : res.err;
            emit
            self->errorOccurred(QStringLiteral("Warp Connect Error"), msg);
        }
    });
    watcher->setFuture(future);
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliConnectAsync() {
    return runCommandAsync("warp-cli", {"connect"}, 15000);
}

void MainFunctions::cliDisconnect() {
    if (isConnecting || isDisconnecting)
        return;
    isDisconnecting = true;
    if (!isServiceActive()) {
        isDisconnecting = false;
        return;
    }
    QFuture<CommandResult> future = cliDisconnectAsync();
    QPointer < MainFunctions > self(this);
    auto watcher = new QFutureWatcher<CommandResult>(this);
    connect(watcher, &QFutureWatcher<CommandResult>::finished, watcher, [self, watcher]() {
        const CommandResult res = watcher->result();
        watcher->deleteLater();
        if (!self) return;

        self->isDisconnecting = false;
        if (res.timedOut || res.exitCode != 0 || !res.out.contains("Success", Qt::CaseInsensitive)) {
            const QString msg = res.err.isEmpty() ? QStringLiteral("Disconnection failed or timed out.") : res.err;
            emit
            self->errorOccurred(QStringLiteral("Warp Disconnect Error"), msg);
        }
    });
    watcher->setFuture(future);
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliDisconnectAsync() {
    return runCommandAsync("warp-cli", {"disconnect"}, 15000);
    // why the fuck does warp-cli reason this as "settings changed", but not for the connect..?
}

void MainFunctions::cliRegister() {
    QPointer < MainFunctions > self(this);
    auto watcher = new QFutureWatcher<CommandResult>(this);
    watcher->setFuture(runCommandAsync("warp-cli", {"--accept-tos", "registration", "new"}, 15000));

    connect(watcher, &QFutureWatcher<CommandResult>::finished, watcher, [self, watcher]() {
        const CommandResult res = watcher->result();
        watcher->deleteLater();
        if (!self) return;

        if (res.timedOut) {
            emit
            self->errorOccurred(QStringLiteral("Registration Error"), QStringLiteral("Registration timed out."));
            return;
        }

        if (res.exitCode != 0) {
            const QString msg = res.err.isEmpty() ? res.out : res.err;
            emit
            self->errorOccurred(QStringLiteral("Registration Error"),
                                msg.isEmpty() ? QStringLiteral("Registration failed.") : msg);
            return;
        }

        const QString message = res.out.isEmpty() ? QStringLiteral("Registration completed.") : res.out;
        emit
        self->infoOccurred(QStringLiteral("Registration"), message);
    });
}

QString MainFunctions::cliStatus() {
    return runCommand("warp-cli", {"status"});
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliStatusAsync(int timeoutMs) {
    return runCommandAsync("warp-cli", {"status"}, timeoutMs);
}

bool MainFunctions::isServiceActive() {
    QProcess process;
    process.start("systemctl", {"is-active", "--quiet", "warp-svc"});
    if (!process.waitForFinished(3000)) {
        process.kill();
        process.waitForFinished(500);
        return false;
    }

    if (process.exitCode() == 0) {
        return true;
    }

    emit errorOccurred("Service Error",
                       "The 'warp-svc' service is not running.\n\n"
                       "Please enable it by running:\n"
                       "pkexec systemctl start warp-svc");
    return false;
}

std::map<QString, QString> SettingsModeOutputNormalized = {
    {"Warp", "warp"},
    {"DnsOverHttps", "doh"},
    {"WarpWithDnsOverHttps", "warp+doh"},
    {"DnsOverTls", "dot"},
    {"WarpWithDnsOverTls", "warp+dot"},
    {"WarpProxy", "proxy"}, // not sure how well this works
    {"TunnelOnly", "tunnel_only"},
    {"PostureOnly", "Device Information Only?"},
    // cloudflare docs fucking suck and idk how else this will function for some modes..
};

QString MainFunctions::GetCurrentMode() {
    CommandResult output = runCommandResult("warp-cli", {"settings"});

    QRegularExpression re(R"(Mode:\s*([A-Za-z0-9]+))");
    QRegularExpressionMatch match = re.match(output.out);
    if (match.hasMatch()) {
        QString outmatch = match.captured(1);

        auto it = SettingsModeOutputNormalized.find(outmatch);
        return it != SettingsModeOutputNormalized.end() ? it->second : QString();
    }
    return QString();
}

void MainFunctions::refreshCachedMode() {
    cachedMode = GetCurrentMode();
}

bool MainFunctions::isWarpConnected() {
    // DNS-only modes don't create a CloudflareWARP interface
    // Check resolv.conf for local DNS proxy instead
    if (cachedMode == "doh" || cachedMode == "dot") {
        QFile file("/etc/resolv.conf");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        // WARP uses 127.0.0.2 or 127.0.2.2 as local DNS proxy
        return content.contains("127.0.0.2") || content.contains("127.0.2.2");
    }

    /*

    // Broken & unreliable method, and nobody uses this shit anyways, might fix at some point

    // Proxy mode creates a local SOCKS5 proxy on port 40000
    // Check /proc/net/tcp for listening port (40000 = 0x9C40)
    // cloudflare docs dont state much so i do belive the 40000 port is the only option?
    if (cachedMode == "proxy")
    {
        QFile file("/proc/net/tcp");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return false;
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        // Look for 127.0.0.1:40000 listening (0100007F:9C40 in hex, state 0A = LISTEN)
        return content.contains("0100007F:9C40") || content.contains("00000000:9C40");
    }*/

    // Tunnel modes (warp, warp+doh, warp+dot, tunnel_only)
    // check for the CloudflareWARP interface
    QProcess process;
    process.start("ip", {"addr", "show", "CloudflareWARP"});
    if (!process.waitForFinished(2000)) {
        process.kill();
        process.waitForFinished(500);
        return false;
    }
    return (process.exitCode() == 0);

    // other modes like device posture only dont do shit anyways and are for org usage
    // i dont really think there is anything to check there?? idk
}