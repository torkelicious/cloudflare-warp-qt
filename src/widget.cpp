#include "widget.h"
#include "settingsdiag.h"
#include <QSettings>
#include <QFutureWatcher>
#include <array>
#include "./ui_widget.h"

static constexpr std::array<int, 6> kPollDelays = {500, 1000, 2000, 3000, 4000, 5000};

static const QString kPrivateHtml = QStringLiteral(
    "<html><body><p><span style='font-size:11pt; color:#b0b0b0;'>Your "
    "internet is </span><span style='font-size:11pt; font-weight:600; "
    "color:#F48120;'>private</span></p></body></html>");

static const QString kNotPrivateHtml = QStringLiteral(
    "<html><body><p><span style='font-size:11pt; color:#b0b0b0;'>Your "
    "internet is </span><span style='font-size:11pt; font-weight:600; "
    "color:#ffffff;'>not private</span></p></body></html>");

Widget::Widget(MainFunctions *mf, QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget), mf(mf), connectedState(false), shouldUnfocus(false),
      pendingState(TransitionState::None), pollTimer(new QTimer(this)), expectedState(false), pollAttempt(0) {
    ui->setupUi(this);

    refreshSettings();

    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);

    pollTimer->setSingleShot(true);
    connect(pollTimer, &QTimer::timeout, this, &Widget::pollConnectionState);

    const QSettings settings;
    const bool shouldAutoConnect = settings.value("autoConnect", false).toBool();
    const bool actuallyConnected = mf->isWarpConnected();

    if (shouldAutoConnect && !actuallyConnected) {
        mf->cliConnect();
    }

    connectedState = actuallyConnected;
    updateUI();
}

Widget::~Widget() {
    delete ui;
}

void Widget::refreshSettings() {
    const QSettings settings;
    shouldUnfocus = settings.value("minimizeOnUnfocus", true).toBool();

    const int targetW = settings.value("minWidth", 310).toInt();
    const int targetH = settings.value("minHeight", 405).toInt();

    if (settings.value("useFixedSize", false).toBool()) {
        setFixedSize(targetW, targetH);
    } else {
        setMinimumSize(targetW, targetH);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
}

void Widget::openSettings() {
    SettingsDiag dlg(mf, this);
    dlg.exec();
    refreshSettings();
    emit settingsChanged();
}

void Widget::on_btn_settings_clicked() {
    openSettings();
}

void Widget::closeEvent(QCloseEvent *event) {
    // Keep in mem instead
    hide();
    event->ignore();
}

bool Widget::event(QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate && shouldUnfocus) {
        hide();
        return true;
    }
    return QWidget::event(event);
}

void Widget::updateUI() const {
    if (pendingState == TransitionState::Connecting) {
        ui->btn_start->setEnabled(false);
        ui->btn_start->setText("Connecting...");
        ui->connected_status->setText("CONNECTING...");
        ui->connected_status->setStyleSheet("color: #F48120; font-weight: bold;");
        ui->sub_status->setText("Please wait...");
        ui->btn_start->setStyleSheet(
            "QPushButton { background-color: #FAAD3F; color: #ffffff; "
            "padding: 15px 32px; border-radius: 20px; font-weight: bold; font-size: 18px; border: none; }");
        return;
    }
    if (pendingState == TransitionState::Disconnecting) {
        ui->btn_start->setEnabled(false);
        ui->btn_start->setText("Disconnecting...");
        ui->connected_status->setText("DISCONNECTING...");
        ui->connected_status->setStyleSheet("color: #ffffff; font-weight: bold;");
        ui->sub_status->setText("Please wait...");
        ui->btn_start->setStyleSheet(
            "QPushButton { background-color: #FAAD3F; color: #ffffff; "
            "padding: 15px 32px; border-radius: 20px; font-weight: bold; font-size: 18px; border: none; }");
        return;
    }

    ui->btn_start->setEnabled(true);

    if (connectedState) {
        ui->btn_start->setText("Disconnect");
        ui->connected_status->setText("CONNECTED");
        ui->connected_status->setStyleSheet("color: #F48120; font-weight: bold;");
        ui->sub_status->setText(getPrivateHtml());

        ui->btn_start->setStyleSheet(
            "QPushButton { background-color: #F48120; color: #ffffff; "
            "padding: 15px 32px; border-radius: 20px; font-weight: bold; font-size: 18px; border: none; } "
            "QPushButton:hover { background-color: #FAAD3F; }");
    } else {
        ui->btn_start->setText("Connect");
        ui->connected_status->setText("DISCONNECTED");
        ui->connected_status->setStyleSheet("color: #ffffff; font-weight: bold;");
        ui->sub_status->setText(getNotPrivateHtml());

        ui->btn_start->setStyleSheet(
            "QPushButton { background-color: #ffffff; color: #404041; "
            "padding: 15px 32px; border-radius: 20px; font-weight: bold; font-size: 18px; border: 3px solid #404041; } "
            "QPushButton:hover { background-color: #FAAD3F; color: #ffffff; border: 3px solid #FAAD3F; }");
    }
}

void Widget::pollConnectionState() {
    if (const bool reality = mf->isWarpConnected(); reality == expectedState || pollAttempt >= kPollDelays.size()) {
        connectedState = reality;
        setPending(TransitionState::None);
        emit connectionChanged(connectedState);
        updateUI();
        return;
    }

    const int delay = kPollDelays[pollAttempt++];
    pollTimer->start(delay);
}

void Widget::on_btn_start_clicked() {
    // early return to prevent rapid spam clicking
    if (pendingState != TransitionState::None) {
        return;
    }

    setPending(connectedState ? TransitionState::Disconnecting : TransitionState::Connecting);
    updateUI();

    auto *watcher = new QFutureWatcher<MainFunctions::CommandResult>(this);
    if (!connectedState) {
        watcher->setFuture(mf->cliConnectAsync());
    } else {
        watcher->setFuture(mf->cliDisconnectAsync());
    }

    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        watcher->deleteLater();
        expectedState = !connectedState;
        pollAttempt = 0;
        const int initialDelay = expectedState ? 2000 : 800;
        pollTimer->start(initialDelay);
    });
}

void Widget::onConnectionChanged(const bool connected) {
    if (connectedState != connected) {
        connectedState = connected;
        setPending(TransitionState::None);
        updateUI();
    }
}

void Widget::setPending(const TransitionState state) {
    pendingState = state;
}

QString Widget::getPrivateHtml() {
    return kPrivateHtml;
}

QString Widget::getNotPrivateHtml() {
    return kNotPrivateHtml;
}
