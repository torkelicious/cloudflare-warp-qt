#ifndef SETTINGSDIAG_H
#define SETTINGSDIAG_H

#include <QDialog>
#include <QSettings>
#include <QSpinBox>
#include "mainfunctions.h"

class QCheckBox;
class QComboBox;
class QPushButton;

class SettingsDiag : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDiag(MainFunctions *mf, QWidget *parent = nullptr);

private slots:
    void saveSettings();

    void registerNewClient();

    void enableDaemon();

    void disableOfficialTray();

    void resetSettings();

private:
    void setupUI();

    void loadSettings() const;

    static void setAutoStart(bool enable);

    QCheckBox *checkAutoStart = nullptr;
    QCheckBox *checkAutoConnect = nullptr;
    QCheckBox *checkShowOnStart = nullptr;
    QCheckBox *checkMinimizeOnUnfocus = nullptr;
    QComboBox *comboMode = nullptr;
    QSpinBox *pollingRateSpinBox = nullptr;
    QSpinBox *minimumWindowWidthSpinBox = nullptr;
    QSpinBox *minimumWindowHeightSpinBox = nullptr;
    QCheckBox *checkUseMinAsFixedSize = nullptr;
    QPushButton *btnRegister = nullptr;
    QPushButton *btnEnableDaemon = nullptr;
    QPushButton *btnDisableOfficialTray = nullptr;
    QPushButton *btnReset = nullptr;
    MainFunctions *mf = nullptr;
    QSettings settings;
};

#endif // SETTINGSDIAG_H
