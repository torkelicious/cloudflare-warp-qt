#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QAction>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QPointer>
#include "mainfunctions.h"
#include "widget.h"

class SysTray : public QObject {
    Q_OBJECT

public:
    explicit SysTray(MainFunctions *mf, QObject *parent = nullptr);

    Widget *ensureWidget();

public
    slots:

    

    void handleErrorBackoff(const QString &title, const QString &message);

    void setupTray();

public
    slots:

    

    void updateStatus(bool connected);

    void checkStatus();

    void showErrorNotification(const QString &title, const QString &message);

    void showInfoNotification(const QString &title, const QString &message);

    signals:

    

    void connectionChanged(bool connected);

private:
    QSystemTrayIcon *trayIcon;
    QPointer<Widget> popupWidget;
    MainFunctions *mf;
    QAction *toggleAction;
    QTimer *pollTimer;
    bool lastKnownState;

    QIcon iconConnected;
    QIcon iconDisconnected;

    // Toggle polling state
    QTimer *togglePollTimer;
    bool toggleExpectedState;
    int togglePollAttempt;

    void pollToggleState();

    void startToggle();
};

#endif // SYSTRAY_H