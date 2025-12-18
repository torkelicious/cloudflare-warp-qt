#include "systray.h"
#include <QApplication>
#include <QMenu>
#include <QFutureWatcher>

SysTray::SysTray(Widget *widget, QObject *parent)
    : QObject(parent)
      , popupWidget(widget)
      , lastKnownState(false) {
    lastKnownState = mf.isWarpConnected();

    setupTray();

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &SysTray::checkStatus);
    pollTimer->start(5000);
}

void SysTray::checkStatus() {
    bool actualState = mf.isWarpConnected();

    if (actualState != lastKnownState) {
        lastKnownState = actualState;
        emit connectionChanged(actualState);
        updateStatus(actualState);
    }
}

void SysTray::setupTray() {
    trayIcon = new QSystemTrayIcon(this);

    QMenu *menu = new QMenu(popupWidget);

    toggleAction = new QAction("Connect", this);

    connect(toggleAction, &QAction::triggered, [this]() {
        toggleAction->setEnabled(false);
        if (pollTimer && pollTimer->isActive()) pollTimer->stop();
        if (lastKnownState) {
            toggleAction->setText("Disconnecting...");
            trayIcon->setToolTip("Warp: Disconnecting...");
        } else {
            toggleAction->setText("Connecting...");
            trayIcon->setToolTip("Warp: Connecting...");
        }

        auto watcher = new QFutureWatcher<MainFunctions::CommandResult>(this);
        if (lastKnownState) {
            watcher->setFuture(mf.cliDisconnectAsync());
        } else {
            watcher->setFuture(mf.cliConnectAsync());
        }
        connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
            watcher->deleteLater();
            const bool expected = !lastKnownState;
            int attempt = 0;
            const int initialDelay = expected ? 2000 : 800; // wait a bit before first check
            std::function<void()> poll = [this, expected, &attempt, &poll]() mutable {
                bool reality = mf.isWarpConnected();
                if (reality == expected) {
                    if (reality != lastKnownState) {
                        lastKnownState = reality;
                        emit connectionChanged(reality);
                    }
                    updateStatus(reality);
                    toggleAction->setEnabled(true);
                    if (pollTimer) pollTimer->start(5000);
                    return;
                }
                static const int delays[] = {500, 1000, 2000, 3000, 4000, 5000};
                if (attempt >= int(sizeof(delays) / sizeof(delays[0]))) {
                    if (reality != lastKnownState) {
                        lastKnownState = reality;
                        emit connectionChanged(reality);
                    }
                    updateStatus(reality);
                    toggleAction->setEnabled(true);
                    if (pollTimer) pollTimer->start(5000);
                    return;
                }
                int delay = delays[attempt++];
                QTimer::singleShot(delay, this, poll);
            };
            QTimer::singleShot(initialDelay, this, poll);
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
