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
#include <QSpinBox>
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QDebug>

SettingsDiag::SettingsDiag(MainFunctions *mf, QWidget *parent)
    : QDialog(parent), mf(mf) {
    setWindowTitle("Settings");
    resize(360, 460);
    setupUI();
    loadSettings();
}

void SettingsDiag::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);

    auto *groupGeneral = new QGroupBox("General", this);
    auto *generalLayout = new QVBoxLayout(groupGeneral);

    checkAutoStart = new QCheckBox("Start App on System Boot", this);
    checkAutoConnect = new QCheckBox("Auto-Connect WARP on Start", this);
    checkShowOnStart = new QCheckBox("Show Window on App Start", this);
    checkMinimizeOnUnfocus = new QCheckBox("Minimize Popup on Unfocus", this);

    checkAutoStart->setToolTip("Automatically starts this GUI application when you log into your desktop environment.");
    checkAutoConnect->setToolTip("Automatically connects to WARP when this application starts.");
    checkShowOnStart->setToolTip(
        "Shows the main window when the application starts, instead of minimizing to the system tray.");
    checkMinimizeOnUnfocus->setToolTip("Hides the main window automatically when you click outside of it.");

    generalLayout->addWidget(checkAutoStart);
    generalLayout->addWidget(checkAutoConnect);
    generalLayout->addWidget(checkShowOnStart);
    generalLayout->addWidget(checkMinimizeOnUnfocus);
    mainLayout->addWidget(groupGeneral);

    auto *groupSystem = new QGroupBox("Troubleshooting", this);
    auto *systemLayout = new QVBoxLayout(groupSystem);

    btnEnableDaemon = new QPushButton("Enable warp-svc (WARP Daemon)", this);
    btnEnableDaemon->setToolTip("Requires root: Enables and starts the background 'warp-svc' system service.");

    btnDisableOfficialTray = new QPushButton("Disable/Kill Official Tray", this);
    btnDisableOfficialTray->setToolTip(
        "Disables the user unit 'warp-taskbar' and kills the process if running. May require root.");

    systemLayout->addWidget(btnEnableDaemon);
    systemLayout->addWidget(btnDisableOfficialTray);
    mainLayout->addWidget(groupSystem);

    auto *groupWarp = new QGroupBox("WARP Configuration", this);
    auto *warpLayout = new QFormLayout(groupWarp);

    comboMode = new QComboBox(this);
    comboMode->addItems({"warp", "doh", "warp+doh", "dot", "warp+dot", "tunnel_only"});
    comboMode->setToolTip(
        "Selects the operation mode for Cloudflare WARP. Changing this will apply the new mode via the CLI.");

    btnRegister = new QPushButton("Register New Device", this);
    btnRegister->setToolTip("Registers a new device with Cloudflare WARP. Warning: This may reset your license key.");

    warpLayout->addRow("Operation Mode:", comboMode);
    warpLayout->addRow(btnRegister);
    mainLayout->addWidget(groupWarp);

    auto *groupAdvanced = new QGroupBox("Advanced", this);
    auto *advancedLayout = new QFormLayout(groupAdvanced);

    pollingRateSpinBox = new QSpinBox(this);
    pollingRateSpinBox->setRange(100, 100000);
    pollingRateSpinBox->setSingleStep(50);
    pollingRateSpinBox->setSuffix(" ms");
    pollingRateSpinBox->setToolTip(
        "Sets how often the system tray icon checks the WARP daemon for status changes (in milliseconds).");

    minimumWindowWidthSpinBox = new QSpinBox(this);
    minimumWindowWidthSpinBox->setSingleStep(1);
    minimumWindowWidthSpinBox->setSuffix(" px");
    minimumWindowWidthSpinBox->setRange(100, 4000);
    minimumWindowWidthSpinBox->setToolTip("Sets the minimum width of the main window in pixels.");

    minimumWindowHeightSpinBox = new QSpinBox(this);
    minimumWindowHeightSpinBox->setSingleStep(1);
    minimumWindowHeightSpinBox->setSuffix(" px");
    minimumWindowHeightSpinBox->setRange(100, 4000);
    minimumWindowHeightSpinBox->setToolTip("Sets the minimum height of the main window in pixels.");

    advancedLayout->addRow("System Tray Polling Rate:", pollingRateSpinBox);

    auto *minSizeLayout = new QHBoxLayout;
    minSizeLayout->addWidget(minimumWindowWidthSpinBox);
    minSizeLayout->addWidget(new QLabel("×"));
    minSizeLayout->addWidget(minimumWindowHeightSpinBox);
    minSizeLayout->setContentsMargins(0, 0, 0, 0);
    advancedLayout->addRow("Window Size:", minSizeLayout);

    checkUseMinAsFixedSize = new QCheckBox("Lock Window to this Size", this);
    checkUseMinAsFixedSize->setToolTip(
        "Locks the main window to the width and height specified above, preventing resizing, may require restarting app.");

    advancedLayout->addRow(checkUseMinAsFixedSize);

    mainLayout->addWidget(groupAdvanced);

    auto *btnLayout = new QHBoxLayout;

    btnReset = new QPushButton("Reset to Defaults", this);
    btnReset->setToolTip("Resets all application settings to their original default values.");

    auto *btnSave = new QPushButton("Save", this);
    auto *btnCancel = new QPushButton("Cancel", this);

    btnLayout->addWidget(btnReset);
    btnLayout->addStretch();
    btnLayout->addWidget(btnSave);
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);

    connect(btnSave, &QPushButton::clicked, this, &SettingsDiag::saveSettings);
    connect(btnCancel, &QPushButton::clicked, this, &SettingsDiag::reject);
    connect(btnReset, &QPushButton::clicked, this, &SettingsDiag::resetSettings);
    connect(btnRegister, &QPushButton::clicked, this, &SettingsDiag::registerNewClient);
    connect(btnEnableDaemon, &QPushButton::clicked, this, &SettingsDiag::enableDaemon);
    connect(btnDisableOfficialTray, &QPushButton::clicked, this, &SettingsDiag::disableOfficialTray);
}

void SettingsDiag::loadSettings() const {
    checkAutoConnect->setChecked(settings.value("autoConnect", false).toBool());
    checkAutoStart->setChecked(settings.value("autoStart", false).toBool());
    checkShowOnStart->setChecked(settings.value("showOnStart", false).toBool());
    checkMinimizeOnUnfocus->setChecked(settings.value("minimizeOnUnfocus", false).toBool());
    pollingRateSpinBox->setValue(settings.value("trayPollingRate", 5000).toInt());
    minimumWindowWidthSpinBox->setValue(settings.value("minWidth", 310).toInt());
    minimumWindowHeightSpinBox->setValue(settings.value("minHeight", 405).toInt());
    checkUseMinAsFixedSize->setChecked(settings.value("useFixedSize", false).toBool());

    const QString mode = mf ? mf->GetCurrentMode() : QString();

    if (!mode.isEmpty()) {
        if (const int idx = comboMode->findText(mode, Qt::MatchExactly); idx >= 0) {
            comboMode->setCurrentIndex(idx);
        } else {
            comboMode->addItem(mode);
            comboMode->setCurrentIndex(comboMode->count() - 1);
        }
    }
}

void SettingsDiag::saveSettings() {
    settings.setValue("autoConnect", checkAutoConnect->isChecked());
    settings.setValue("autoStart", checkAutoStart->isChecked());
    settings.setValue("showOnStart", checkShowOnStart->isChecked());
    settings.setValue("minimizeOnUnfocus", checkMinimizeOnUnfocus->isChecked());
    settings.setValue("trayPollingRate", pollingRateSpinBox->value());
    settings.setValue("minWidth", minimumWindowWidthSpinBox->value());
    settings.setValue("minHeight", minimumWindowHeightSpinBox->value());
    settings.setValue("useFixedSize", checkUseMinAsFixedSize->isChecked());

    setAutoStart(checkAutoStart->isChecked());
    const QString currentMode = mf ? mf->GetCurrentMode() : QString();

    if (const QString selectedMode = comboMode->currentText(); !selectedMode.isEmpty() && currentMode.compare(
                                                                   selectedMode, Qt::CaseInsensitive) != 0) {
        if (mf) {
            mf->runCommand("warp-cli", {"mode", selectedMode});
            mf->refreshCachedMode();
        }
    }
    accept();
}

void SettingsDiag::resetSettings() {
    auto reply = QMessageBox::question(
        this,
        "Reset Settings",
        "Are you sure you want to reset all settings to their default values?\n\nThis cannot be undone.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        settings.clear();
        setAutoStart(false);
        loadSettings();
        QMessageBox::information(this, "Settings Reset",
                                 "Settings have been restored to defaults.\n\nClick 'Save' to apply them, or 'Cancel' to abort.");
    }
}

void SettingsDiag::registerNewClient() {
    auto reply = QMessageBox::question(
        this,
        "Register",
        "This will re-register the client and might reset your license key. Continue?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        MainFunctions::cliRegister();
    }
}

void SettingsDiag::enableDaemon() {
    btnEnableDaemon->setEnabled(false);
    btnEnableDaemon->setText("Waiting for authentication...");

    auto *watcher = new QFutureWatcher<MainFunctions::CommandResult>(this);
    if (!mf) {
        watcher->deleteLater();
        btnEnableDaemon->setEnabled(true);
        btnEnableDaemon->setText("Enable warp-svc (WARP Daemon)");
        return;
    }
    watcher->setFuture(mf->runCommandAsync("pkexec", {"systemctl", "enable", "--now", "warp-svc"}, 120000));
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher] {
        const auto [exitCode, out, err, timedOut] = watcher->future().result();
        watcher->deleteLater();

        btnEnableDaemon->setEnabled(true);
        btnEnableDaemon->setText("Enable warp-svc (WARP Daemon)");

        if (!timedOut && exitCode == 0) {
            QMessageBox::information(this, "Success",
                                     "'warp-svc' system service enabled and started.");
        } else {
            const QString errMsg = !err.isEmpty()
                                       ? err
                                       : (!out.isEmpty()
                                              ? out
                                              : (timedOut
                                                     ? "Timed out waiting for authentication or command to finish"
                                                     : "Unknown error"));
            QMessageBox::warning(this, "Operation Failed",
                                 QString("Failed to enable/start 'warp-svc'.\n\nDetails:\n%1").arg(errMsg));
        }
    });
}

void SettingsDiag::setAutoStart(const bool enable) {
    const QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    if (const bool created = QDir().mkpath(autostartDir); !created) {
        qWarning() << "Failed to create autostart directory";
    }
    const QString desktopPath = autostartDir + "/cloudflare-warp-qt.desktop";

    if (enable) {
        QFile resFile(":/cloudflare-warp-qt.desktop");
        if (!resFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open /resources desktop file";
            return;
        }

        QFile outFile(desktopPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to write desktop file to autostart";
            return;
        }

        outFile.write(resFile.readAll());
        outFile.close();
        resFile.close();

        QFile::Permissions perms = outFile.permissions();
        perms |= QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;
        outFile.setPermissions(perms);
    } else {
        QFile file(desktopPath);
        if (file.exists()) {
            if (!file.remove()) {
                qWarning() << "Failed to remove autostart desktop file!";
            }
        }
    }
}

void SettingsDiag::disableOfficialTray() {
    btnDisableOfficialTray->setEnabled(false);
    btnDisableOfficialTray->setText("Disabling tray...");

    auto *watcher1 = new QFutureWatcher<MainFunctions::CommandResult>(this);
    if (!mf) {
        watcher1->deleteLater();
        btnDisableOfficialTray->setEnabled(true);
        btnDisableOfficialTray->setText("Disable/Kill Official Tray");
        return;
    }
    watcher1->setFuture(
        mf->runCommandAsync("systemctl", {"--user", "disable", "warp-taskbar"}, 10000));

    connect(watcher1, &QFutureWatcherBase::finished, this, [this, watcher1] {
        const auto res1 = watcher1->future().result();
        watcher1->deleteLater();

        if (!mf) return;
        auto *watcher2 = new QFutureWatcher<MainFunctions::CommandResult>(this);
        watcher2->setFuture(
            mf->runCommandAsync("systemctl", {"--user", "stop", "warp-taskbar"}, 5000)
        );

        // Passed `this` as context to prevent dangling execution if dialog is closed
        connect(watcher2, &QFutureWatcherBase::finished, this, [this, res1, watcher2] {
            watcher2->deleteLater();

            btnDisableOfficialTray->setEnabled(true);
            btnDisableOfficialTray->setText("Disable/Kill Official Tray");

            const QString autostartDir =
                    QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                    + "/autostart";

            QDir().mkpath(autostartDir);

            const QString desktopPath =
                    autostartDir + "/com.cloudflare.WarpTaskbar.desktop";

            QFile file(desktopPath);
            const bool ok = file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);

            if (ok) {
                QTextStream out(&file);
                out <<
                        "[Desktop Entry]\n"
                        "Type=Application\n"
                        "Name=Cloudflare WARP Zero Trust client / Tray Override\n"
                        "Hidden=true\n";
                file.close();
            }

            if (!res1.timedOut &&
                (res1.exitCode == 0 || res1.exitCode == 1) &&
                ok) {
                QMessageBox::information(
                    this,
                    "Success",
                    "WARP tray disabled and autostart overridden for this user."
                );
            } else {
                QMessageBox::warning(
                    this,
                    "Partial/Failed",
                    "User service was handled, but autostart override failed."
                );
            }
        });
    });
}
