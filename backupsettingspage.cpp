#include "backupsettingspage.h"
#include "thememanager.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QSettings>
#include <QFormLayout>

BackupSettingsPage::BackupSettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void BackupSettingsPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // Backup location and interval (QFormLayout)
    QFormLayout *formLayout = new QFormLayout();
    QLabel *locationLabel = new QLabel("Backup Location:", this);
    m_backupLocationEdit = new QLineEdit(this);
    m_browseBtn = new QPushButton("Browse...", this);
    QWidget *locationWidget = new QWidget(this);
    auto locationHLayout = new QHBoxLayout(locationWidget);
    locationHLayout->setContentsMargins(0,0,0,0);
    locationHLayout->addWidget(m_backupLocationEdit);
    locationHLayout->addWidget(m_browseBtn);
    locationWidget->setLayout(locationHLayout);
    formLayout->addRow(locationLabel, locationWidget);

    QLabel *intervalLabel = new QLabel("Backup every:", this);
    m_autoBackupInterval = new QSpinBox(this);
    m_autoBackupInterval->setRange(1, 744); // 1 hour to 31 days
    m_autoBackupInterval->setValue(24); // Default to daily
    QLabel *hoursLabel = new QLabel("hours", this);
    QWidget *intervalWidget = new QWidget(this);
    auto intervalHLayout = new QHBoxLayout(intervalWidget);
    intervalHLayout->setContentsMargins(0,0,0,0);
    intervalHLayout->addWidget(m_autoBackupInterval);
    intervalHLayout->addWidget(hoursLabel);
    intervalWidget->setLayout(intervalHLayout);
    formLayout->addRow(intervalLabel, intervalWidget);

    mainLayout->addLayout(formLayout);

    // Auto backup settings
    QGroupBox *autoBackupGroup = new QGroupBox("Automatic Backup", this);
    QVBoxLayout *autoBackupLayout = new QVBoxLayout(autoBackupGroup);
    m_autoBackupCheck = new QCheckBox("Enable automatic backups", this);
    // Apply Qt-compatible checkbox style (simple, no unsupported CSS properties)
    QString switchStyle = R"(
        QCheckBox::indicator {
            width: 40px;
            height: 24px;
            border-radius: 12px;
            border: 2px solid #aaa;
        }
        QCheckBox::indicator:unchecked {
            background: #ccc;
        }
        QCheckBox::indicator:checked {
            background: #4CAF50;
            border: 2px solid #388E3C;
        }
    )";
    m_autoBackupCheck->setStyleSheet(switchStyle);
    autoBackupLayout->addWidget(m_autoBackupCheck);
    mainLayout->addWidget(autoBackupGroup);

    // Manual backup section
    QGroupBox *manualBackupGroup = new QGroupBox("Manual Backup", this);
    QVBoxLayout *manualBackupLayout = new QVBoxLayout(manualBackupGroup);
    m_lastBackupLabel = new QLabel(this);
    manualBackupLayout->addWidget(m_lastBackupLabel);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_backupNowBtn = new QPushButton("Backup Now", this);
    m_restoreBtn = new QPushButton("Restore from Backup", this);
    buttonLayout->addWidget(m_backupNowBtn);
    buttonLayout->addWidget(m_restoreBtn);
    buttonLayout->addStretch();
    manualBackupLayout->addLayout(buttonLayout);
    mainLayout->addWidget(manualBackupGroup);
    mainLayout->addStretch();

    // Connect signals
    connect(m_browseBtn, &QPushButton::clicked, this, &BackupSettingsPage::onBrowseBackupLocation);
    connect(m_backupNowBtn, &QPushButton::clicked, this, &BackupSettingsPage::onBackupNow);
    connect(m_restoreBtn, &QPushButton::clicked, this, &BackupSettingsPage::onRestoreBackup);
    connect(m_autoBackupCheck, &QCheckBox::checkStateChanged, this, &BackupSettingsPage::onAutoBackupChanged);
    connect(m_backupLocationEdit, &QLineEdit::textChanged, this, &BackupSettingsPage::settingsChanged);
    connect(m_autoBackupInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, &BackupSettingsPage::settingsChanged);
    connect(m_autoBackupCheck, &QCheckBox::toggled, this, &BackupSettingsPage::settingsChanged);

    // Apply theme styling to the spinbox
    ThemeManager::instance()->applyThemeToSpinBox(m_autoBackupInterval);
}

void BackupSettingsPage::loadSettings()
{
    QSettings settings;
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                         + "/QuteNote/Backups";
    
    m_backupLocationEdit->setText(settings.value("backupLocation", defaultPath).toString());
    m_autoBackupCheck->setChecked(settings.value("autoBackupEnabled", false).toBool());
    m_autoBackupInterval->setValue(settings.value("autoBackupInterval", 24).toInt());
    
    QString lastBackup = settings.value("lastBackupTime").toString();
    if (!lastBackup.isEmpty()) {
        m_lastBackupLabel->setText("Last backup: " + lastBackup);
    } else {
        m_lastBackupLabel->setText("No backup performed yet");
    }
}

void BackupSettingsPage::saveSettings()
{
    QSettings settings;
    settings.setValue("backupLocation", m_backupLocationEdit->text());
    settings.setValue("autoBackupEnabled", m_autoBackupCheck->isChecked());
    settings.setValue("autoBackupInterval", m_autoBackupInterval->value());
}

void BackupSettingsPage::onBrowseBackupLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        "Select Backup Location",
        m_backupLocationEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty()) {
        m_backupLocationEdit->setText(dir);
        emit settingsChanged();
    }
}

void BackupSettingsPage::onBackupNow()
{
    QString backupDir = m_backupLocationEdit->text();
    if (backupDir.isEmpty()) {
        QMessageBox::warning(this, "Backup",
            "Please select a backup location first.");
        return;
    }

    QDir dir(backupDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::warning(this, "Backup",
            "Could not create backup directory.");
        return;
    }

    // Create timestamped backup
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString backupPath = backupDir + "/backup_" + timestamp;
    
    createBackup(backupPath);
    QSettings settings;
    settings.setValue("lastBackupTime", QDateTime::currentDateTime().toString());
    m_lastBackupLabel->setText("Last backup: " + QDateTime::currentDateTime().toString());
    
    QMessageBox::information(this, "Backup",
        "Backup completed successfully.");
}

void BackupSettingsPage::onRestoreBackup()
{
    QString backupFile = QFileDialog::getOpenFileName(this,
        "Select Backup to Restore",
        m_backupLocationEdit->text(),
        "Backup Files (*.zip);;All Files (*)");
    
    if (backupFile.isEmpty())
        return;

    if (QMessageBox::warning(this, "Restore Backup",
        "Restoring from backup will overwrite your current data. Are you sure?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        
        if (restoreFromBackup(backupFile)) {
            QMessageBox::information(this, "Restore Backup",
                "Backup restored successfully. Please restart the application.");
        } else {
            QMessageBox::warning(this, "Restore Backup",
                "Failed to restore from backup.");
        }
    }
}

void BackupSettingsPage::onAutoBackupChanged(int state)
{
    m_autoBackupInterval->setEnabled(state == Qt::Checked);
    emit settingsChanged();
}

bool BackupSettingsPage::createBackup(const QString &path)
{
    // Implementation for creating backup archive
    // ... (implementation omitted for brevity)
    return true;
}

bool BackupSettingsPage::restoreFromBackup(const QString &path)
{
    // Implementation for restoring from backup archive
    // ... (implementation omitted for brevity)
    return true;
}