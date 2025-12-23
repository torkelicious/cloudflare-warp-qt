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
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFutureWatcher>

SettingsDiag::SettingsDiag(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    resize(320, 400);
    setupUI();
    loadSettings();
}

void SettingsDiag::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupGeneral = new QGroupBox("General", this);
    QVBoxLayout *generalLayout = new QVBoxLayout(groupGeneral);

    checkAutoStart = new QCheckBox("Start App on System Boot", this);
    checkAutoConnect = new QCheckBox("Auto-Connect WARP on Start", this);
    checkShowOnStart = new QCheckBox("Show Window on App Start", this);
    checkMinimizeOnUnfocus = new QCheckBox("Minimize the popup on Unfocus", this);

    generalLayout->addWidget(checkAutoStart);
    generalLayout->addWidget(checkAutoConnect);
    generalLayout->addWidget(checkShowOnStart);
    generalLayout->addWidget(checkMinimizeOnUnfocus);
    mainLayout->addWidget(groupGeneral);

    QGroupBox *groupSystem = new QGroupBox("Troubleshooting", this);
    QVBoxLayout *systemLayout = new QVBoxLayout(groupSystem);

    btnEnableDaemon = new QPushButton("Enable warp-svc (Warp Daemon)", this);
    btnEnableDaemon->setToolTip("Requires root: Enables and starts 'warp-svc' system service.");

    btnDisableOfficialTray = new QPushButton("Disable/Kill Official Tray", this);
    btnDisableOfficialTray->setToolTip(
        "Disables user unit 'warp-taskbar' and kills process if running, may require root.");

    systemLayout->addWidget(btnEnableDaemon);
    systemLayout->addWidget(btnDisableOfficialTray);
    mainLayout->addWidget(groupSystem);

    QGroupBox *groupWarp = new QGroupBox("Warp Configuration", this);
    QFormLayout *warpLayout = new QFormLayout(groupWarp);

    comboMode = new QComboBox(this);
    comboMode->addItems({"warp", "doh", "warp+doh", "dot", "warp+dot", "proxy"});

    btnRegister = new QPushButton("Register New Device", this);

    warpLayout->addRow("Operation Mode:", comboMode);
    warpLayout->addRow(btnRegister);
    mainLayout->addWidget(groupWarp);

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

void SettingsDiag::loadSettings()
{
    checkAutoConnect->setChecked(settings.value("autoConnect", false).toBool());
    checkAutoStart->setChecked(settings.value("autoStart", false).toBool());
    checkShowOnStart->setChecked(settings.value("showOnStart", false).toBool());
    checkMinimizeOnUnfocus->setChecked(settings.value("minimizeOnUnfocus", true).toBool());

    QString status = mf.runCommand("warp-cli", {"status"});
    QRegularExpression re("^Mode:\\s*([^\n]+)", QRegularExpression::MultilineOption);
    QRegularExpressionMatch m = re.match(status);
    if (m.hasMatch())
    {
        QString mode = m.captured(1).trimmed();
        int idx = comboMode->findText(mode, Qt::MatchExactly);
        if (idx >= 0)
        {
            comboMode->setCurrentIndex(idx);
        }
    }
}

void SettingsDiag::saveSettings()
{
    settings.setValue("autoConnect", checkAutoConnect->isChecked());
    settings.setValue("autoStart", checkAutoStart->isChecked());
    settings.setValue("showOnStart", checkShowOnStart->isChecked());
    settings.setValue("minimizeOnUnfocus", checkMinimizeOnUnfocus->isChecked());
    setAutoStart(checkAutoStart->isChecked());
    QString status = mf.runCommand("warp-cli", {"status"});
    QRegularExpression re("^Mode:\\s*([^\n]+)", QRegularExpression::MultilineOption);
    QRegularExpressionMatch m = re.match(status);
    QString currentMode = m.hasMatch() ? m.captured(1).trimmed() : QString();
    QString selectedMode = comboMode->currentText();
    if (!selectedMode.isEmpty() && currentMode.compare(selectedMode, Qt::CaseInsensitive) != 0)
    {
        mf.runCommand("warp-cli", {"set-mode", selectedMode});
    }
    accept();
}

void SettingsDiag::registerNewClient()
{
    auto reply = QMessageBox::question(
        this,
        "Register",
        "This will re-register the client and might reset your license key. Continue?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        mf.cliRegister();
    }
}

void SettingsDiag::enableDaemon()
{
    auto watcher = new QFutureWatcher<MainFunctions::CommandResult>(this);
    watcher->setFuture(mf.runCommandAsync("pkexec", {"systemctl", "enable", "--now", "warp-svc"}, 120000));
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]()
            {
        auto res = watcher->future().result();
        watcher->deleteLater();
        if (!res.timedOut && res.exitCode == 0) {
            QMessageBox::information(this, "Success",
                                     "'warp-svc' system service enabled and started.");
        } else {
            QString err = res.err;
            if (err.isEmpty()) err = res.out;
            if (err.isEmpty())
                err = res.timedOut
                          ? "Timed out waiting for authentication or command to finish"
                          : "Unknown error";
            QMessageBox::warning(this, "Operation Failed",
                                 QString("Failed to enable/start 'warp-svc'.\n\nDetails:\n%1").arg(err));
        } });
}

void SettingsDiag::setAutoStart(bool enable)
{
    // (~/.config/autostart)
    const QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(autostartDir);

    const QString desktopPath = autostartDir + "/cloudflare-warp-qt.desktop";

    if (enable)
    {
        QFile resFile(":/cloudflare-warp-qt.desktop");
        if (!resFile.open(QIODevice::ReadOnly))
        {
            qWarning() << "Failed to open /resources desktop file";
            return;
        }

        QFile outFile(desktopPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qWarning() << "Failed to write desktop file to autostart";
            return;
        }

        outFile.write(resFile.readAll());
        outFile.close();
        resFile.close();

        // make the desktop file executable
        QFile::Permissions perms = outFile.permissions();
        perms |= QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;
        outFile.setPermissions(perms);
    }
    else
    {
        QFile file(desktopPath);
        if (file.exists())
        {
            if (!file.remove())
            {
                qWarning() << "Failed to remove autostart desktop file!";
            }
        }
    }
}

void SettingsDiag::disableOfficialTray()
{
    auto watcher1 = new QFutureWatcher<MainFunctions::CommandResult>(this);
    watcher1->setFuture(
        mf.runCommandAsync("systemctl", {"--user", "disable", "warp-taskbar"}, 10000));

    connect(watcher1, &QFutureWatcherBase::finished, this, [this, watcher1]()
            {
        auto res1 = watcher1->future().result();
        watcher1->deleteLater();

        auto watcher2 = new QFutureWatcher<MainFunctions::CommandResult>(this);
        watcher2->setFuture(
            mf.runCommandAsync("systemctl", {"--user", "stop", "warp-taskbar"}, 5000)
            );

        connect(watcher2, &QFutureWatcherBase::finished, this, [this, res1, watcher2]() {
            watcher2->deleteLater();

            // Per-user autostart override: always write Hidden=true
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
                    "Warp tray disabled and autostart overridden for this user."
                    );
            } else {
                QMessageBox::warning(
                    this,
                    "Partial/Failed",
                    "User service was handled, but autostart override failed."
                    );
            }
        }); });
}
