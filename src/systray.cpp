#include "systray.h"
#include <QApplication>
#include <QMenu>
#include <QSettings>
#include <QFutureWatcher>
#include <QDebug>
#include <QAction>
#include <QTimer>
#include <array>

static constexpr std::array<int, 6> kPollDelays = {500, 1000, 2000, 3000, 4000, 5000};

SysTray::SysTray(MainFunctions *mf, QObject *parent)
    : QObject(parent), trayIcon(nullptr), popupWidget(nullptr), mf(mf), toggleAction(nullptr), lastKnownState(false),
      togglePollTimer(new QTimer(this)), toggleExpectedState(false), togglePollAttempt(0) {
    iconConnected = QIcon(":/icons/connected.png");
    iconDisconnected = QIcon(":/icons/disconnected.png");

    connect(this->mf, &MainFunctions::infoOccurred, this, &SysTray::showInfoNotification);
    connect(this->mf, &MainFunctions::errorOccurred, this, &SysTray::showErrorNotification);
    connect(this->mf, &MainFunctions::errorOccurred, this, &SysTray::handleErrorBackoff);

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &SysTray::checkStatus);

    loadSettings();
    pollTimer->start(systrayPollRate);

    togglePollTimer->setSingleShot(true);
    connect(togglePollTimer, &QTimer::timeout, this, &SysTray::pollToggleState);
}

SysTray::~SysTray() {
    delete popupWidget;
}

void SysTray::loadSettings() {
    const QSettings settings;
    systrayPollRate = settings.value("trayPollingRate", 5000).toInt();
    if (pollTimer && pollTimer->isActive()) {
        pollTimer->start(systrayPollRate);
    }
}

Widget *SysTray::ensureWidget() {
    if (!popupWidget) {
        popupWidget = new Widget(mf);
        connect(this, &SysTray::connectionChanged, popupWidget, &Widget::onConnectionChanged);
        connect(popupWidget, &Widget::settingsChanged, this, &SysTray::loadSettings);
    }
    return popupWidget;
}

void SysTray::checkStatus() {
    if (const bool actualState = mf->isWarpConnected(); actualState != lastKnownState) {
        lastKnownState = actualState;
        emit connectionChanged(actualState);
        updateStatus(actualState);
    }
}

void SysTray::startToggle() {
    if (!mf->isServiceActive()) return;

    toggleAction->setEnabled(false);
    if (pollTimer && pollTimer->isActive()) {
        pollTimer->stop(); // Stop periodic checks while we run commands
    }

    if (lastKnownState) {
        toggleAction->setText("Disconnecting...");
        trayIcon->setToolTip("Warp: Disconnecting...");
    } else {
        toggleAction->setText("Connecting...");
        trayIcon->setToolTip("Warp: Connecting...");
    }

    toggleExpectedState = !lastKnownState;
    togglePollAttempt = 0;

    auto *watcher = new QFutureWatcher<MainFunctions::CommandResult>(this);
    if (!lastKnownState) {
        watcher->setFuture(mf->cliConnectAsync());
    } else {
        watcher->setFuture(mf->cliDisconnectAsync());
    }

    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        watcher->deleteLater();
        const int initialDelay = toggleExpectedState ? 2000 : 800;
        togglePollTimer->start(initialDelay);
    });
}

void SysTray::pollToggleState() {
    const bool reality = mf->isWarpConnected();

    if (reality == toggleExpectedState || togglePollAttempt >= kPollDelays.size()) {
        if (reality != lastKnownState) {
            lastKnownState = reality;
            emit connectionChanged(reality);
        }
        updateStatus(reality);
        toggleAction->setEnabled(true);
        if (pollTimer) {
            pollTimer->start(systrayPollRate); // Resume background polling
        }
        return;
    }

    const int delay = kPollDelays[togglePollAttempt++];
    togglePollTimer->start(delay);
}

void SysTray::setupTray() {
    trayIcon = new QSystemTrayIcon(this);
    auto *menu = new QMenu;

    toggleAction = menu->addAction("Connect", this, &SysTray::startToggle);

    menu->addSeparator();

    menu->addAction("Show", [this] { ensureWidget()->show(); });
    menu->addAction("Preferences", [this] { ensureWidget()->openSettings(); });
    menu->addAction("Quit", qApp, &QApplication::quit);

    trayIcon->setContextMenu(menu);

    connect(trayIcon, &QSystemTrayIcon::activated, [this](const QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (Widget *w = ensureWidget(); w->isVisible()) w->hide();
            else w->show();
        }
    });

    updateStatus(lastKnownState);
    trayIcon->show();
}

void SysTray::handleErrorBackoff(const QString &, const QString &) const {
    if (pollTimer) pollTimer->start(10000);
}

void SysTray::updateStatus(const bool connected) const {
    if (connected) {
        toggleAction->setText("Disconnect");
        trayIcon->setIcon(iconConnected);
        trayIcon->setToolTip("Warp: Connected");
    } else {
        toggleAction->setText("Connect");
        trayIcon->setIcon(iconDisconnected);
        trayIcon->setToolTip("Warp: Disconnected");
    }
}

void SysTray::showErrorNotification(const QString &title, const QString &message) const {
    if (trayIcon && trayIcon->isSystemTrayAvailable()) {
        trayIcon->showMessage(title, message, QSystemTrayIcon::Critical, 5000);
    } else {
        qWarning() << "Tray Error (" << title << "):" << message;
    }
}

void SysTray::showInfoNotification(const QString &title, const QString &message) const {
    if (trayIcon && trayIcon->isSystemTrayAvailable()) {
        trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    } else {
        qDebug() << "Tray Info (" << title << "):" << message;
    }
}
