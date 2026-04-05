#ifndef WIDGET_H
#define WIDGET_H

#include <QCloseEvent>
#include <QEvent>
#include <QWidget>
#include <QTimer>
#include "mainfunctions.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class Widget;
}

QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

public:
    explicit Widget(MainFunctions *mf, QWidget *parent = nullptr);

    ~Widget() override;

protected:
    void closeEvent(QCloseEvent *event) override;

    bool event(QEvent *event) override;

public slots:
    void onConnectionChanged(bool connected);

    void openSettings();

private slots:
    void on_btn_start_clicked();

    void on_btn_settings_clicked();

signals:
    void connectionChanged(bool connected);

    void settingsChanged();

private:
    enum class TransitionState { None, Connecting, Disconnecting };

    Ui::Widget *ui;
    MainFunctions *mf;
    bool connectedState;
    bool shouldUnfocus;
    TransitionState pendingState;

    QTimer *pollTimer;
    bool expectedState;
    size_t pollAttempt;

    void pollConnectionState();

    void refreshSettings();

    void updateUI() const;

    void setPending(TransitionState state);

    [[nodiscard]] static QString getPrivateHtml();

    [[nodiscard]] static QString getNotPrivateHtml();
};

#endif // WIDGET_H
