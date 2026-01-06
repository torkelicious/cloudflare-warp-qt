#include "systray.h"
#include <QApplication>
#include <QMenu>
#include <QFutureWatcher>

static const int kPollDelays[] = {500, 1000, 2000, 3000, 4000, 5000};
static constexpr int kPollDelaysCount = sizeof(kPollDelays) / sizeof(kPollDelays[0]);

SysTray::SysTray(MainFunctions *mf, QObject *parent)
    : QObject(parent), trayIcon(nullptr), popupWidget(nullptr), mf(mf), toggleAction(nullptr), lastKnownState(false), togglePollTimer(new QTimer(this)), toggleExpectedState(false), togglePollAttempt(0)
{
    iconConnected = QIcon(":/icons/connected.png");
    iconDisconnected = QIcon(":/icons/disconnected.png");

    connect(this->mf, &MainFunctions::infoOccurred, this, &SysTray::showInfoNotification);
    connect(this->mf, &MainFunctions::errorOccurred, this, &SysTray::handleErrorBackoff);

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &SysTray::checkStatus);
    pollTimer->start(5000);

    togglePollTimer->setSingleShot(true);
    connect(togglePollTimer, &QTimer::timeout, this, &SysTray::pollToggleState);
}

void SysTray::checkStatus()
{
    bool actualState = mf->isWarpConnected();

    if (actualState != lastKnownState)
    {
        lastKnownState = actualState;
        emit connectionChanged(actualState);
        updateStatus(actualState);
    }
}

Widget *SysTray::ensureWidget()
{
    if (!popupWidget)
    {
        popupWidget = new Widget(mf, nullptr);
        connect(popupWidget, &Widget::connectionChanged, this, &SysTray::updateStatus);
        connect(this, &SysTray::connectionChanged, popupWidget, &Widget::onConnectionChanged);
    }
    return popupWidget;
}

void SysTray::pollToggleState()
{
    bool reality = mf->isWarpConnected();

    if (reality == toggleExpectedState || togglePollAttempt >= kPollDelaysCount)
    {
        if (reality != lastKnownState)
        {
            lastKnownState = reality;
            emit connectionChanged(reality);
        }
        updateStatus(reality);
        toggleAction->setEnabled(true);
        if (pollTimer)
            pollTimer->start(5000);
        return;
    }

    int delay = kPollDelays[togglePollAttempt++];
    togglePollTimer->start(delay);
}

void SysTray::startToggle()
{
    toggleAction->setEnabled(false);
    if (pollTimer && pollTimer->isActive())
    {
        pollTimer->stop();
    }
    if (lastKnownState)
    {
        toggleAction->setText("Disconnecting...");
        trayIcon->setToolTip("Warp: Disconnecting...");
    }
    else
    {
        toggleAction->setText("Connecting...");
        trayIcon->setToolTip("Warp: Connecting...");
    }

    auto watcher = new QFutureWatcher<MainFunctions::CommandResult>(this);
    if (lastKnownState)
    {
        watcher->setFuture(mf->cliDisconnectAsync());
    }
    else
    {
        watcher->setFuture(mf->cliConnectAsync());
    }
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]()
            {
        watcher->deleteLater();
        toggleExpectedState = !lastKnownState;
        togglePollAttempt = 0;
        const int initialDelay = toggleExpectedState ? 2000 : 800;
        togglePollTimer->start(initialDelay); });
}

void SysTray::setupTray()
{
    trayIcon = new QSystemTrayIcon(this);

    QMenu *menu = new QMenu();
    trayIcon->setContextMenu(menu);

    toggleAction = new QAction("Connect", this);

    connect(toggleAction, &QAction::triggered, this, &SysTray::startToggle);

    menu->addAction(toggleAction);
    menu->addSeparator();

    menu->addAction("Show", [this]()
                    { ensureWidget()->showPositioned(); });

    menu->addAction("Preferences", [this]()
                    { ensureWidget()->openSettings(); });

    menu->addAction("Quit", qApp, &QApplication::quit);

    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason)
            {
        if (reason == QSystemTrayIcon::Trigger) {
            Widget *w = ensureWidget();
            if (w->isVisible()) {
                w->hide();
            } else {
                w->showPositioned();
            }
        } });

    updateStatus(lastKnownState);
    trayIcon->show();
}

void SysTray::handleErrorBackoff(const QString &, const QString &)
{
    // back off polling after error
    if (pollTimer)
    {
        pollTimer->start(10000);
    }
}

void SysTray::updateStatus(bool connected)
{
    if (connected)
    {
        toggleAction->setText("Disconnect");
        trayIcon->setIcon(iconConnected);
        trayIcon->setToolTip("Warp: Connected");
    }
    else
    {
        toggleAction->setText("Connect");
        trayIcon->setIcon(iconDisconnected);
        trayIcon->setToolTip("Warp: Disconnected");
    }
}

void SysTray::showErrorNotification(const QString &title, const QString &message)
{
    if (trayIcon && trayIcon->isSystemTrayAvailable())
    {
        trayIcon->showMessage(title, message, QSystemTrayIcon::Critical, 5000);
    }
    else
    {
        qWarning() << title << ":" << message;
    }
}

void SysTray::showInfoNotification(const QString &title, const QString &message)
{
    if (trayIcon && trayIcon->isSystemTrayAvailable())
    {
        trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
    }
    else
    {
        qDebug() << title << ":" << message;
    }
}