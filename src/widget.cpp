#include "widget.h"
#include <QApplication>
#include <QCursor>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include "./ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
      , ui(new Ui::Widget)
      , connectedState(false) {
    ui->setupUi(this);
    setFixedSize(310, 405);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);

    // Auto Connect check
    QSettings settings;
    bool shouldAutoConnect = settings.value("autoConnect", false).toBool();
    bool actuallyConnected = mf.isWarpConnected();

    if (shouldAutoConnect && !actuallyConnected) {
        mf.cliConnect();
        // dont update manually, trust the autoupdate
    }

    connectedState = actuallyConnected;
    updateUI();
}

Widget::~Widget() {
    delete ui;
}

void Widget::openSettings() {
    SettingsDiag dlg(this);
    dlg.exec();
}

void Widget::on_btn_settings_clicked() {
    openSettings();
}

void Widget::closeEvent(QCloseEvent *event) {
    hide();
    event->ignore();
}

bool Widget::event(QEvent *event) {
    if (event->type() == QEvent::WindowDeactivate) {
        hide();
        return true;
    }
    return QWidget::event(event);
}

void Widget::showPositioned() {
    QPoint cursor = QCursor::pos();
    QScreen *screen = QGuiApplication::screenAt(cursor);
    if (!screen) screen = QGuiApplication::primaryScreen();

    QRect screenGeom = screen->geometry();

    int x = cursor.x() - (width() / 2);
    int y = cursor.y() - height() - 10;

    if (y < screenGeom.top()) y = cursor.y() + 20;
    if (x < screenGeom.left()) x = screenGeom.left() + 5;
    if (x + width() > screenGeom.right()) x = screenGeom.right() - width() - 5;

    move(x, y);
    show();
    activateWindow();
    raise();
}

void Widget::updateUI() {
    ui->btn_start->setEnabled(true);

    if (connectedState) {
        ui->btn_start->setText("Connected");
        ui->connected_status->setText("CONNECTED");
        ui->connected_status->setStyleSheet("color: #F48120; font-weight: bold;");
        ui->sub_status->setText(getPrivateHtml());

        ui->btn_start->setStyleSheet(
            "QPushButton { background-color: #F48120; color: #ffffff; "
            "padding: 15px 32px; border-radius: 20px; font-weight: bold; font-size: 18px; border: none; } "
            "QPushButton:hover { background-color: #FAAD3F; }");
    } else {
        ui->btn_start->setText("Disconnected");
        ui->connected_status->setText("DISCONNECTED");
        ui->connected_status->setStyleSheet("color: #ffffff; font-weight: bold;");
        ui->sub_status->setText(getNotPrivateHtml());

        ui->btn_start->setStyleSheet(
            "QPushButton { background-color: #ffffff; color: #404041; "
            "padding: 15px 32px; border-radius: 20px; font-weight: bold; font-size: 18px; border: 3px solid #404041; } "
            "QPushButton:hover { background-color: #FAAD3F; color: #ffffff; border: 3px solid #FAAD3F; }");
    }
}

void Widget::on_btn_start_clicked() {
    ui->btn_start->setEnabled(false);

    ui->btn_start->setText(connectedState ? "Disconnecting..." : "Connecting...");
    ui->btn_start->setStyleSheet(
        "QPushButton { background-color: #FAAD3F; color: #ffffff; "
        "padding: 15px 32px; border-radius: 20px; font-weight: bold; font-size: 18px; border: none; }");

    QApplication::processEvents();

    if (!connectedState) {
        mf.cliConnect();
    } else {
        mf.cliDisconnect();
    }

    QTimer::singleShot(1000, this, [this]() {
        bool reality = mf.isWarpConnected();
        if (reality != connectedState) {
            connectedState = reality;
            emit connectionChanged(connectedState);
        }
        updateUI();
    });
}

void Widget::onConnectionChanged(bool connected) {
    if (connectedState != connected) {
        connectedState = connected;
        updateUI();
    }
}

QString Widget::getPrivateHtml() const {
    return "<html><body><p><span style='font-size:11pt; color:#b0b0b0;'>Your "
            "internet is </span><span style='font-size:11pt; font-weight:600; "
            "color:#F48120;'>private</span></p></body></html>";
}

QString Widget::getNotPrivateHtml() const {
    return "<html><body><p><span style='font-size:11pt; color:#b0b0b0;'>Your "
            "internet is </span><span style='font-size:11pt; font-weight:600; "
            "color:#ffffff;'>not private</span></p></body></html>";
}