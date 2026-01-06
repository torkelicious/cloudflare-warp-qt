#ifndef WIDGET_H
#define WIDGET_H

#include <QCloseEvent>
#include <QEvent>
#include <QWidget>
#include <QTimer>
#include "mainfunctions.h"

class SettingsDiag;

QT_BEGIN_NAMESPACE

namespace Ui {
    class Widget;
}

QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

public:
    explicit Widget(MainFunctions *mf, QWidget *parent = nullptr);

    ~Widget();

    void showPositioned();

protected:
    void closeEvent(QCloseEvent *event) override;

    bool event(QEvent *event) override;

private
    slots:


    

    void on_btn_start_clicked();

    void on_btn_settings_clicked();

public
    slots:


    

    void onConnectionChanged(bool connected);

    void openSettings();

    signals:


    

    void connectionChanged(bool connected);

private:
    enum class TransitionState { None, Connecting, Disconnecting };

    Ui::Widget *ui;
    MainFunctions *mf;
    bool connectedState;
    bool shouldUnfocus;
    TransitionState pendingState;

    // Polling state for connection checks
    QTimer *pollTimer;
    bool expectedState;
    int pollAttempt;

    void pollConnectionState();

private:
    void refreshSettings();

    void updateUI();

    void setPending(TransitionState state);

    QString getPrivateHtml() const;

    QString getNotPrivateHtml() const;
};
#endif // WIDGET_H