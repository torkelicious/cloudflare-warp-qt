#ifndef SETTINGSDIAG_H
#define SETTINGSDIAG_H

#include <QDialog>
#include <QSettings>
#include "mainfunctions.h"

class QVBoxLayout;
class QCheckBox;
class QComboBox;
class QPushButton;

class SettingsDiag : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDiag(MainFunctions *mf, QWidget *parent = nullptr);

private slots:

    void saveSettings();

    void registerNewClient();

    void enableDaemon();

    void disableOfficialTray();

private:
    void setupUI();

    void loadSettings();

    void setAutoStart(bool enable);

    QCheckBox *checkAutoStart;
    QCheckBox *checkAutoConnect;
    QCheckBox *checkShowOnStart;
    QCheckBox *checkMinimizeOnUnfocus;
    QComboBox *comboMode;
    QPushButton *btnRegister;
    QPushButton *btnEnableDaemon;
    QPushButton *btnDisableOfficialTray;
    MainFunctions *mf;
    QSettings settings;
};

#endif // SETTINGSDIAG_H