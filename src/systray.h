#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QSystemTrayIcon>
#include <QPointer>
#include "mainfunctions.h"
#include "widget.h"

class QAction;
class QTimer;

class SysTray : public QObject {
    Q_OBJECT

public:
    explicit SysTray(MainFunctions *mf, QObject *parent = nullptr);

    ~SysTray() override;

    Widget *ensureWidget();

    void setupTray();

public slots:
    void loadSettings();

    void handleErrorBackoff(const QString &title, const QString &message) const;

    void updateStatus(bool connected) const;

    void checkStatus();

    void showErrorNotification(const QString &title, const QString &message) const;

    void showInfoNotification(const QString &title, const QString &message) const;

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

    QTimer *togglePollTimer;
    bool toggleExpectedState;
    size_t togglePollAttempt;

    int systrayPollRate = 5000;

    void pollToggleState();

    void startToggle();
};

#endif // SYSTRAY_H
