#ifndef SYSTRAY_H
#define SYSTRAY_H

#include <QAction>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTimer>
#include "mainfunctions.h"
#include "widget.h"

class SysTray : public QObject {
    Q_OBJECT

public:
    explicit SysTray(Widget *widget, QObject *parent = nullptr);

    void setupTray();

public
    slots:
    

    void updateStatus(bool connected);

    void checkStatus();

    signals:
    

    void connectionChanged(bool connected);

private:
    QSystemTrayIcon *trayIcon;
    Widget *popupWidget;
    MainFunctions mf;
    QAction *toggleAction;
    QTimer *pollTimer;
    bool lastKnownState; // Local state tracking instead of static global
};

#endif // SYSTRAY_H