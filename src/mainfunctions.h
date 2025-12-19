#ifndef MAINFUNCTIONS_H
#define MAINFUNCTIONS_H

#include <QString>
#include <QFuture>

class MainFunctions {
public:
    MainFunctions();

    struct CommandResult {
        int exitCode = -1;
        QString out;
        QString err;
        bool timedOut = false;
    };

    QString runCommand(const QString &program, const QStringList &arguments);

    CommandResult runCommandResult(const QString &program,
                                   const QStringList &arguments,
                                   int timeoutMs = 3000);

    QFuture<CommandResult> runCommandAsync(const QString &program,
                                           const QStringList &arguments,
                                           int timeoutMs = 3000);

    void cliConnect();

    QFuture<CommandResult> cliConnectAsync();

    void cliDisconnect();

    QFuture<CommandResult> cliDisconnectAsync();

    void cliRegister();

    QString cliStatus();

    bool isServiceActive();

    bool isWarpConnected();
};

#endif // MAINFUNCTIONS_H