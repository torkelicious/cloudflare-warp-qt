#include "settingsdiag.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>

SettingsDiag::SettingsDiag(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    resize(320, 400); // Increased height for new option
    setupUI();
    loadSettings();
}

void SettingsDiag::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // General Settings
    QGroupBox *groupGeneral = new QGroupBox("General", this);
    QVBoxLayout *generalLayout = new QVBoxLayout(groupGeneral);

    checkAutoStart = new QCheckBox("Start App on System Boot", this);
    checkAutoConnect = new QCheckBox("Auto-Connect WARP on Start", this);
    checkShowOnStart = new QCheckBox("Show Window on App Start", this); // <--- NEW

    generalLayout->addWidget(checkAutoStart);
    generalLayout->addWidget(checkAutoConnect);
    generalLayout->addWidget(checkShowOnStart); // <--- Add to layout
    mainLayout->addWidget(groupGeneral);

    // System Services
    QGroupBox *groupSystem = new QGroupBox("Troubleshooting", this);
    QVBoxLayout *systemLayout = new QVBoxLayout(groupSystem);

    btnFixServices = new QPushButton("Enable Warp Daemon && Kill Official Tray", this);
    btnFixServices->setToolTip("Requires Root. Enables 'warp-svc' and disables 'warp-taskbar'.");
    
    systemLayout->addWidget(btnFixServices);
    mainLayout->addWidget(groupSystem);

    // Warp Configuration
    QGroupBox *groupWarp = new QGroupBox("Warp Configuration", this);
    QFormLayout *warpLayout = new QFormLayout(groupWarp);

    comboMode = new QComboBox(this);
    comboMode->addItems({"warp", "doh", "warp+doh", "dot", "warp+dot", "proxy"});

    btnRegister = new QPushButton("Register New Device", this);

    warpLayout->addRow("Operation Mode:", comboMode);
    warpLayout->addRow(btnRegister);
    mainLayout->addWidget(groupWarp);

    // Bottom Buttons
    QPushButton *btnClose = new QPushButton("Close", this);
    mainLayout->addWidget(btnClose);

    // Connect signals
    connect(btnClose, &QPushButton::clicked, this, &SettingsDiag::saveSettings);
    connect(btnRegister, &QPushButton::clicked, this, &SettingsDiag::registerNewClient);
    connect(btnFixServices, &QPushButton::clicked, this, &SettingsDiag::fixSystemServices);
}

void SettingsDiag::loadSettings()
{
    // Load state from QSettings
    checkAutoConnect->setChecked(settings.value("autoConnect", false).toBool());
    checkAutoStart->setChecked(settings.value("autoStart", false).toBool());
    checkShowOnStart->setChecked(settings.value("showOnStart", false).toBool()); // <--- Load
}

void SettingsDiag::saveSettings()
{
    settings.setValue("autoConnect", checkAutoConnect->isChecked());
    settings.setValue("autoStart", checkAutoStart->isChecked());
    settings.setValue("showOnStart", checkShowOnStart->isChecked()); // <--- Save

    setAutoStart(checkAutoStart->isChecked());

    QString mode = comboMode->currentText();
    mf.runCommand("warp-cli", {"set-mode", mode});

    accept();
}

void SettingsDiag::setAutoStart(bool enable)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir dir(configDir);
    if (!dir.exists("autostart"))
        dir.mkdir("autostart");

    QString desktopFile = configDir + "/autostart/CloudflareWarpQt.desktop";
    QFile file(desktopFile);

    if (enable) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=CloudflareWarpQt\n";
            out << "Exec=CloudflareWarpQt\n"; 
            out << "X-GNOME-Autostart-enabled=true\n";
            file.close();
        }
    } else {
        if (file.exists())
            file.remove();
    }
}

void SettingsDiag::registerNewClient()
{
    auto reply = QMessageBox::question(
        this,
        "Register",
        "This will re-register the client and might reset your license key. Continue?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        mf.cliRegister();
    }
}

void SettingsDiag::fixSystemServices()
{
    QString cmd = "systemctl enable --now warp-svc && "
                  "(systemctl disable --user --now warp-taskbar || true) && "
                  "(pkill -f warp-taskbar || true)";

    QProcess process;
    process.start("pkexec", {"sh", "-c", cmd});
    process.waitForFinished(-1);

    if (process.exitCode() == 0) {
        QMessageBox::information(this, "Success", 
            "Services configured successfully.\n\n"
            "- 'warp-svc' enabled\n"
            "- 'warp-taskbar' disabled/killed");
    } else {
        QMessageBox::warning(this, "Operation Cancelled", 
            "Could not configure services.\n"
            "Either the password was incorrect or the action was cancelled.");
    }
}