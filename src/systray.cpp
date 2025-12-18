#include "systray.h"
#include <QApplication>
#include <QMenu>

SysTray::SysTray(Widget *widget, QObject *parent)
    : QObject(parent)
    , popupWidget(widget)
    , lastKnownState(false) 
{
    // Initialize state
    lastKnownState = mf.isWarpConnected();
    
    setupTray();

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &SysTray::checkStatus);
    pollTimer->start(5000);
}

void SysTray::checkStatus() {
    bool actualState = mf.isWarpConnected();

    // Only update if state has changed
    if (actualState != lastKnownState) {
        lastKnownState = actualState;
        emit connectionChanged(actualState);
        updateStatus(actualState);
    }
}

void SysTray::setupTray() {
    trayIcon = new QSystemTrayIcon(this); // Parented to this (SysTray)

    QMenu *menu = new QMenu(popupWidget); //  QMenu usually needs a parent or to be deleted manually

    toggleAction = new QAction("Connect", this);

    connect(toggleAction, &QAction::triggered, [this]() {
        // optimistic UI update
        if (lastKnownState) {
            mf.cliDisconnect();
        } else {
            mf.cliConnect();
        }
        
        // wait briefly then check reality
        QTimer::singleShot(1000, this, [this](){
            checkStatus();
        });
    });

    menu->addAction(toggleAction);
    menu->addSeparator();

    menu->addAction("Show", [this]() {
        popupWidget->showPositioned();
    });

    menu->addAction("Preferences", [this]() { popupWidget->openSettings(); });

    menu->addAction("Quit", qApp, &QApplication::quit);

    trayIcon->setContextMenu(menu);

    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (popupWidget->isVisible()) {
                popupWidget->hide();
            } else {
                popupWidget->showPositioned();
            }
        }
    });
    
    updateStatus(lastKnownState);
    trayIcon->show();
}

void SysTray::updateStatus(bool connected) {
    if (connected) {
        toggleAction->setText("Disconnect");
        trayIcon->setIcon(QIcon(":/icons/connected.png"));
        trayIcon->setToolTip("Warp: Connected");
    } else {
        toggleAction->setText("Connect");
        trayIcon->setIcon(QIcon(":/icons/disconnected.png"));
        trayIcon->setToolTip("Warp: Disconnected");
    }
}
