#ifndef SETTINGSDIAG_H
#define SETTINGSDIAG_H

#include <QDialog>
#include <QSettings>
#include "mainfunctions.h"

// Forward declarations to speed up compilation
class QVBoxLayout;
class QCheckBox;
class QComboBox;
class QPushButton;

class SettingsDiag : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDiag(QWidget *parent = nullptr);

private slots:
    void saveSettings();
    void registerNewClient();
    void fixSystemServices();

private:
    void setupUI();
    void loadSettings();
    void setAutoStart(bool enable);

    QCheckBox *checkAutoStart;
    QCheckBox *checkAutoConnect;
    QCheckBox *checkShowOnStart; // <--- NEW CHECKBOX
    QComboBox *comboMode;
    QPushButton *btnRegister;
    QPushButton *btnFixServices;
    MainFunctions mf;
    QSettings settings;
};

#endif // SETTINGSDIAG_H