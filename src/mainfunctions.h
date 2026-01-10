#ifndef MAINFUNCTIONS_H
#define MAINFUNCTIONS_H

#include <QString>
#include <QFuture>
#include <QObject>

class MainFunctions : public QObject
{
    Q_OBJECT

public:
    explicit MainFunctions(QObject *parent = nullptr);

    struct CommandResult
    {
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

    QFuture<CommandResult> cliStatusAsync(int timeoutMs = 3000);

    bool isServiceActive();

    QString GetCurrentMode();

    void refreshCachedMode();

    bool isWarpConnected();

signals:

    void errorOccurred(const QString &title, const QString &message);

    void infoOccurred(const QString &title, const QString &message);

private:
    bool isConnecting = false;
    bool isDisconnecting = false;
    QString cachedMode;
};

#endif // MAINFUNCTIONS_H