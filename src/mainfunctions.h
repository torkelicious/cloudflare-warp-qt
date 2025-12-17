#ifndef MAINFUNCTIONS_H
#define MAINFUNCTIONS_H

#include <QString>

class MainFunctions {
public:
    MainFunctions();

    // Helper to run commands using QProcess
    QString runCommand(const QString &program, const QStringList &arguments);

    void cliConnect();
    void cliDisconnect();
    void cliRegister();

    QString cliStatus();
    bool isServiceActive();
    bool isWarpConnected();
};

#endif // MAINFUNCTIONS_H