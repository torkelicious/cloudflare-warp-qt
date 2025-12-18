#include "mainfunctions.h"
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QtConcurrent>

MainFunctions::MainFunctions() {
}

QString MainFunctions::runCommand(const QString &program, const QStringList &arguments) {
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(3000)) {
        return QString();
    }
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

MainFunctions::CommandResult MainFunctions::runCommandResult(const QString &program,
                                                             const QStringList &arguments,
                                                             int timeoutMs) {
    QProcess process;
    process.start(program, arguments);

    CommandResult res;
    if (!process.waitForFinished(timeoutMs)) {
        res.timedOut = true;
        res.exitCode = -1;
        return res;
    }

    res.exitCode = process.exitCode();
    res.out = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    res.err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    return res;
}

QFuture<MainFunctions::CommandResult> MainFunctions::runCommandAsync(const QString &program,
                                                                     const QStringList &arguments,
                                                                     int timeoutMs) {
    return QtConcurrent::run([=]() {
        return runCommandResult(program, arguments, timeoutMs);
    });
}

void MainFunctions::cliConnect() {
    QString output = runCommand("warp-cli", {"connect"});
    QWidget *parent = QApplication::activeWindow();

    if (!output.contains("Success", Qt::CaseInsensitive) && !output.isEmpty()) {
        if (!isServiceActive()) return;

        QMessageBox::warning(parent, "Warp Error", output);
    }
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliConnectAsync() {
    return runCommandAsync("warp-cli", {"connect"}, 15000);
}

void MainFunctions::cliDisconnect() {
    QString output = runCommand("warp-cli", {"disconnect"});
    QWidget *parent = QApplication::activeWindow();

    if (!output.contains("Success", Qt::CaseInsensitive) && !output.isEmpty()) {
        QMessageBox::warning(parent, "Warp Error", output);
    }
}

QFuture<MainFunctions::CommandResult> MainFunctions::cliDisconnectAsync() {
    return runCommandAsync("warp-cli", {"disconnect"}, 15000);
}

void MainFunctions::cliRegister() {
    QString output = runCommand("warp-cli", {"registration", "new"});
    QWidget *parent = QApplication::activeWindow();
    QMessageBox::information(parent, "Registration", output);
}

QString MainFunctions::cliStatus() {
    return runCommand("warp-cli", {"status"});
}

bool MainFunctions::isServiceActive() {
    int exitCode = QProcess::execute("systemctl", {"is-active", "--quiet", "warp-svc"});

    if (exitCode == 0) {
        return true;
    }

    QWidget *parent = QApplication::activeWindow();
    QMessageBox::critical(parent, "Service Error",
                          "The 'warp-svc' service is not running.\n\n"
                          "Please enable it by running:\n"
                          "pkexec systemctl start warp-svc");
    return false;
}

bool MainFunctions::isWarpConnected() {
    int code = QProcess::execute("ip", {"addr", "show", "CloudflareWARP"});
    return (code == 0);
}
