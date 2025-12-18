#ifndef WIDGET_H
#define WIDGET_H

#include <QCloseEvent>
#include <QEvent>
#include <QWidget>
#include "mainfunctions.h"
#include "settingsdiag.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    void showPositioned();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void on_btn_start_clicked();
    void on_btn_settings_clicked(); // <--- New Slot

public slots:
    void onConnectionChanged(bool connected);
    void openSettings();

signals:
    void connectionChanged(bool connected);

private:
    Ui::Widget *ui;
    MainFunctions mf;
    bool connectedState;

    void updateUI();
    QString getPrivateHtml() const; 
    QString getNotPrivateHtml() const;
};
#endif // WIDGET_H
