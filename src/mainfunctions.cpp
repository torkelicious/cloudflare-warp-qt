#include "mainfunctions.h"
#include <QProcess>
#include <QMessageBox>
#include <QDebug>

MainFunctions::MainFunctions() {}

QString MainFunctions::runCommand(const QString &program, const QStringList &arguments) {
    QProcess process;
    process.start(program, arguments);
    
    // Wait up to 3 seconds for a response, then fail gracefully
    if (!process.waitForFinished(3000)) {
        return QString();
    }
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

void MainFunctions::cliConnect() {
    QString output = runCommand("warp-cli", {"connect"});

    // Only show error if it fails and output isn't empty (or Success)
    if (!output.contains("Success", Qt::CaseInsensitive) && !output.isEmpty()) {
         // Check service if connection failed
        if (!isServiceActive()) return; 

        QMessageBox::warning(nullptr, "Warp Error", output);
    }
}

void MainFunctions::cliDisconnect() {
    QString output = runCommand("warp-cli", {"disconnect"});

    if (!output.contains("Success", Qt::CaseInsensitive) && !output.isEmpty()) {
        QMessageBox::warning(nullptr, "Warp Error", output);
    }
}

void MainFunctions::cliRegister() {
    QString output = runCommand("warp-cli", {"registration", "new"});
    QMessageBox::information(nullptr, "Registration", output);
}

QString MainFunctions::cliStatus() {
    return runCommand("warp-cli", {"status"});
}

bool MainFunctions::isServiceActive() {
    // QProcess::execute returns 0 on success (Active)
    int exitCode = QProcess::execute("systemctl", {"is-active", "--quiet", "warp-svc"});

    if (exitCode == 0) {
        return true;
    }

    QMessageBox::critical(nullptr, "Service Error",
                          "The 'warp-svc' service is not running.\n\n"
                          "Please enable it by running:\n"
                          "sudo systemctl start warp-svc");
    return false;
}

bool MainFunctions::isWarpConnected() {
    // check specific interface existence using QProcess
    QProcess process;
    process.start("ip", {"addr", "show", "CloudflareWARP"});
    process.waitForFinished(500); // Should be instant

    // If exit code is 0, interface exists. If 1, it does not.
    return (process.exitCode() == 0);
}
