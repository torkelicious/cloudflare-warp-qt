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
#include <QHBoxLayout>
#include <QSpinBox>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFutureWatcher>

static const QRegularExpression kModeRegex(QStringLiteral("^Mode:\\s*([^\n]+)"),
                                           QRegularExpression::MultilineOption);

SettingsDiag::SettingsDiag(MainFunctions *mf, QWidget *parent)
    : QDialog(parent), mf(mf) {
    setWindowTitle("Settings");
    resize(360, 460);
    setupUI();
    loadSettings();
}

void SettingsDiag::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupGeneral = new QGroupBox("General", this);
    QVBoxLayout *generalLayout = new QVBoxLayout(groupGeneral);

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

    QGroupBox *groupSystem = new QGroupBox("Troubleshooting", this);
    QVBoxLayout *systemLayout = new QVBoxLayout(groupSystem);

    btnEnableDaemon = new QPushButton("Enable warp-svc (WARP Daemon)", this);
    btnEnableDaemon->setToolTip("Requires root: Enables and starts the background 'warp-svc' system service.");

    btnDisableOfficialTray = new QPushButton("Disable/Kill Official Tray", this);
    btnDisableOfficialTray->setToolTip(
        "Disables the user unit 'warp-taskbar' and kills the process if running. May require root.");

    systemLayout->addWidget(btnEnableDaemon);
    systemLayout->addWidget(btnDisableOfficialTray);
    mainLayout->addWidget(groupSystem);

    QGroupBox *groupWarp = new QGroupBox("WARP Configuration", this);
    QFormLayout *warpLayout = new QFormLayout(groupWarp);

    comboMode = new QComboBox(this);
    comboMode->addItems({"warp", "doh", "warp+doh", "dot", "warp+dot", "tunnel_only"});
    comboMode->setToolTip(
        "Selects the operation mode for Cloudflare WARP. Changing this will apply the new mode via the CLI.");
    // "proxy" mode is unsupported, so it is not in this list.
    // and nobody uses it anyways...

    btnRegister = new QPushButton("Register New Device", this);
    btnRegister->setToolTip("Registers a new device with Cloudflare WARP. Warning: This may reset your license key.");

    warpLayout->addRow("Operation Mode:", comboMode);
    warpLayout->addRow(btnRegister);
    mainLayout->addWidget(groupWarp);

    QGroupBox *groupAdvanced = new QGroupBox("Advanced", this);
    QFormLayout *advancedLayout = new QFormLayout(groupAdvanced);

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

    QHBoxLayout *minSizeLayout = new QHBoxLayout;
    minSizeLayout->addWidget(minimumWindowWidthSpinBox);
    minSizeLayout->addWidget(new QLabel("×"));
    minSizeLayout->addWidget(minimumWindowHeightSpinBox);
    minSizeLayout->setContentsMargins(0, 0, 0, 0);
    advancedLayout->addRow("Minimum Window Size:", minSizeLayout);

    checkUseMinAsFixedSize = new QCheckBox("Set Minimum Size as Fixed Size", this);
    checkUseMinAsFixedSize->setToolTip(
        "Prevents the window from being resized by locking it to the specified minimum dimensions.");

    advancedLayout->addRow(checkUseMinAsFixedSize);

    mainLayout->addWidget(groupAdvanced);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnSave = new QPushButton("Save", this);
    QPushButton *btnCancel = new QPushButton("Cancel", this);
    btnLayout->addStretch();
    btnLayout->addWidget(btnSave);
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);

    connect(btnSave, &QPushButton::clicked, this, &SettingsDiag::saveSettings);
    connect(btnCancel, &QPushButton::clicked, this, &SettingsDiag::reject);
    connect(btnRegister, &QPushButton::clicked, this, &SettingsDiag::registerNewClient);
    connect(btnEnableDaemon, &QPushButton::clicked, this, &SettingsDiag::enableDaemon);
    connect(btnDisableOfficialTray, &QPushButton::clicked, this, &SettingsDiag::disableOfficialTray);
}

void SettingsDiag::loadSettings() {
    checkAutoConnect->setChecked(settings.value("autoConnect", false).toBool());
    checkAutoStart->setChecked(settings.value("autoStart", false).toBool());
    checkShowOnStart->setChecked(settings.value("showOnStart", false).toBool());
    checkMinimizeOnUnfocus->setChecked(settings.value("minimizeOnUnfocus", false).toBool());
    pollingRateSpinBox->setValue(settings.value("trayPollingRate", 5000).toInt());
    minimumWindowWidthSpinBox->setValue(settings.value("minWidth", 310).toInt());
    minimumWindowHeightSpinBox->setValue(settings.value("minHeight", 405).toInt());
    checkUseMinAsFixedSize->setChecked(settings.value("useFixedSize", false).toBool());

    QString mode = mf ? mf->GetCurrentMode() : QString();
    int idx = comboMode->findText(mode, Qt::MatchExactly);
    if (idx >= 0) {
        comboMode->setCurrentIndex(idx);
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
    QString currentMode = mf ? mf->GetCurrentMode() : QString();
    QString selectedMode = comboMode->currentText();
    if (!selectedMode.isEmpty() && currentMode.compare(selectedMode, Qt::CaseInsensitive) != 0) {
        if (mf) {
            mf->runCommand("warp-cli", {"mode", selectedMode});
            mf->refreshCachedMode();
        }
    }
    accept();
}

void SettingsDiag::registerNewClient() {
    auto reply = QMessageBox::question(
        this,
        "Register",
        "This will re-register the client and might reset your license key. Continue?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (mf)
            mf->cliRegister();
    }
}

void SettingsDiag::enableDaemon() {
    auto watcher = new QFutureWatcher<MainFunctions::CommandResult>(this);
    if (!mf) {
        watcher->deleteLater();
        return;
    }
    watcher->setFuture(mf->runCommandAsync("pkexec", {"systemctl", "enable", "--now", "warp-svc"}, 120000));
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
        auto res = watcher->future().result();
        watcher->deleteLater();
        if (!res.timedOut && res.exitCode == 0) {
            QMessageBox::information(this, "Success",
                                     "'warp-svc' system service enabled and started.");
        } else {
            QString err = !res.err.isEmpty()
                              ? res.err
                              : (!res.out.isEmpty()
                                     ? res.out
                                     : (res.timedOut
                                            ? "Timed out waiting for authentication or command to finish"
                                            : "Unknown error"));
            QMessageBox::warning(this, "Operation Failed",
                                 QString("Failed to enable/start 'warp-svc'.\n\nDetails:\n%1").arg(err));
        }
    });
}

void SettingsDiag::setAutoStart(bool enable) {
    const QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(autostartDir);

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
    auto watcher1 = new QFutureWatcher<MainFunctions::CommandResult>(this);
    if (!mf) {
        watcher1->deleteLater();
        return;
    }
    watcher1->setFuture(
        mf->runCommandAsync("systemctl", {"--user", "disable", "warp-taskbar"}, 10000));

    connect(watcher1, &QFutureWatcherBase::finished, this, [this, watcher1]() {
        auto res1 = watcher1->future().result();
        watcher1->deleteLater();

        if (!mf) return;
        auto watcher2 = new QFutureWatcher<MainFunctions::CommandResult>(this);
        watcher2->setFuture(
            mf->runCommandAsync("systemctl", {"--user", "stop", "warp-taskbar"}, 5000)
        );

        connect(watcher2, &QFutureWatcherBase::finished, this, [this, res1, watcher2]() {
            watcher2->deleteLater();

            const QString autostartDir =
                    QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                    + "/autostart";

            QDir().mkpath(autostartDir);

            const QString desktopPath =
                    autostartDir + "/com.cloudflare.WarpTaskbar.desktop";

            QFile file(desktopPath);
            bool ok = file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);

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
