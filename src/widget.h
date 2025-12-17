#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QEvent>
#include <QCloseEvent>
#include "mainfunctions.h"

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

public slots:
    void onConnectionChanged(bool connected);

signals:
    void connectionChanged(bool connected);

private:
    Ui::Widget *ui;
    MainFunctions mf; // Using PascalCase class
    bool connectedState;

    void updateUI();
    
    // Made const because they don't modify state
    QString getPrivateHtml() const; 
    QString getNotPrivateHtml() const;
};
#endif // WIDGET_H