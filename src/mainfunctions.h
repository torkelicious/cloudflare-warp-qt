#ifndef MAINFUNCTIONS_H
#define MAINFUNCTIONS_H

#include <QFuture>
#include <QObject>
#include <QFileSystemWatcher>
#include <QString>

class MainFunctions : public QObject {
    Q_OBJECT

public:
    explicit MainFunctions(QObject *parent = nullptr);

    struct CommandResult {
        int exitCode = -1;
        QString out;
        QString err;
        bool timedOut = false;
    };

    QString runCommand(const QString &program, const QStringList &arguments);

    static CommandResult runCommandResult(const QString &program,
                                          const QStringList &arguments,
                                          const int timeoutMs = 3000);

    static QFuture<CommandResult> runCommandAsync(const QString &program,
                                                  const QStringList &arguments,
                                                  const int timeoutMs = 3000);

    void cliConnect();

    QFuture<CommandResult> cliConnectAsync();

    void cliDisconnect();

    QFuture<CommandResult> cliDisconnectAsync();

    static void cliRegister();

    QString cliStatus();

    QFuture<CommandResult> cliStatusAsync(const int timeoutMs = 3000);

    bool isServiceActive();

    QString GetCurrentMode();

    void refreshCachedMode();

    bool isWarpConnected() const;

signals:
    void errorOccurred(const QString &title, const QString &message);

    void infoOccurred(const QString &title, const QString &message);

private slots:
    void checkResolvConf();

private:
    void setupToggleOperation(const QFuture<CommandResult> &future, bool isConnect);

    bool isConnecting = false;
    bool isDisconnecting = false;
    QString cachedMode;

    QFileSystemWatcher *resolvWatcher;
    bool resolvConnected = false;
};

#endif // MAINFUNCTIONS_H
