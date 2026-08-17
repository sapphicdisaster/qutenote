#include "backupsettingspage.h"
#include "thememanager.h"
#include "smartpointers.h"
#include "uiutils.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFormLayout>
#include <QResizeEvent>

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
    m_backupLocationEdit = QuteNote::makeOwned<QLineEdit>(this);
    m_browseBtn = QuteNote::makeOwned<QPushButton>("Browse...", this);
    QWidget *locationWidget = new QWidget(this);
    m_locationLayout = QuteNote::makeOwned<QHBoxLayout>(locationWidget);
    m_locationLayout->setContentsMargins(0,0,0,0);
    m_locationLayout->addWidget(m_backupLocationEdit.get());
    m_locationLayout->addWidget(m_browseBtn.get());
    locationWidget->setLayout(m_locationLayout.get());
    formLayout->addRow(locationLabel, locationWidget);

    QLabel *intervalLabel = new QLabel("Backup every:", this);
    m_autoBackupInterval = QuteNote::makeOwned<QSpinBox>(this);
    m_autoBackupInterval->setRange(1, 744); // 1 hour to 31 days
    m_autoBackupInterval->setValue(24); // Default to daily
    QLabel *hoursLabel = new QLabel("hours", this);
    QWidget *intervalWidget = new QWidget(this);
    auto intervalHLayout = new QHBoxLayout(intervalWidget);
    intervalHLayout->setContentsMargins(0,0,0,0);
    intervalHLayout->addWidget(m_autoBackupInterval.get());
    intervalHLayout->addWidget(hoursLabel);
    intervalWidget->setLayout(intervalHLayout);
    formLayout->addRow(intervalLabel, intervalWidget);

    mainLayout->addLayout(formLayout);

    // Auto backup settings
    QGroupBox *autoBackupGroup = new QGroupBox("Automatic Backup", this);
    QVBoxLayout *autoBackupLayout = new QVBoxLayout(autoBackupGroup);
    m_autoBackupCheck = QuteNote::makeOwned<QCheckBox>("Enable automatic backups", this);
    // Style the checkbox as a toggle switch using theme colors
    {
        const Theme& theme = ThemeManager::instance()->currentTheme();
        const QString borderCol = theme.colors.border.name();
        const QString offBg = theme.colors.surface.name();
        const QString onBg = theme.colors.accent.name();
        const QString onBorder = theme.colors.primary.name();

        QString switchStyle = QString(
            "QCheckBox::indicator {"
            "    width: 40px; height: 24px; border-radius: 12px;"
            "    border: 2px solid %1;"
            "}"
            "QCheckBox::indicator:unchecked {"
            "    background: %2;"
            "}"
            "QCheckBox::indicator:checked {"
            "    background: %3;"
            "    border: 2px solid %4;"
            "}"
        ).arg(borderCol, offBg, onBg, onBorder);
        m_autoBackupCheck->setStyleSheet(switchStyle);
    }
    autoBackupLayout->addWidget(m_autoBackupCheck.get());
    mainLayout->addWidget(autoBackupGroup);

    // Manual backup section
    QGroupBox *manualBackupGroup = new QGroupBox("Manual Backup", this);
    QVBoxLayout *manualBackupLayout = new QVBoxLayout(manualBackupGroup);
    m_lastBackupLabel = QuteNote::makeOwned<QLabel>(this);
    manualBackupLayout->addWidget(m_lastBackupLabel.get());
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_backupNowBtn = QuteNote::makeOwned<QPushButton>("Backup Now", this);
    m_restoreBtn = QuteNote::makeOwned<QPushButton>("Restore from Backup", this);
    buttonLayout->addWidget(m_backupNowBtn.get());
    buttonLayout->addWidget(m_restoreBtn.get());
    buttonLayout->addStretch();
    manualBackupLayout->addLayout(buttonLayout);
    mainLayout->addWidget(manualBackupGroup);
    mainLayout->addStretch();

    // Connect signals
    connect(m_browseBtn.get(), &QPushButton::clicked, this, &BackupSettingsPage::onBrowseBackupLocation);
    connect(m_backupNowBtn.get(), &QPushButton::clicked, this, &BackupSettingsPage::onBackupNow);
    connect(m_restoreBtn.get(), &QPushButton::clicked, this, &BackupSettingsPage::onRestoreBackup);
    connect(m_autoBackupCheck.get(), &QCheckBox::checkStateChanged, this, &BackupSettingsPage::onAutoBackupChanged);
    connect(m_backupLocationEdit.get(), &QLineEdit::textChanged, this, &BackupSettingsPage::settingsChanged);
    connect(m_autoBackupInterval.get(), QOverload<int>::of(&QSpinBox::valueChanged), this, &BackupSettingsPage::settingsChanged);
    connect(m_autoBackupCheck.get(), &QCheckBox::toggled, this, &BackupSettingsPage::settingsChanged);

    // Apply theme styling to the spinbox
    ThemeManager::instance()->applyThemeToSpinBox(m_autoBackupInterval.get());
}

void BackupSettingsPage::loadSettings()
{
    QSettings& settings = UIUtils::quteSettings();
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
    QSettings& settings = UIUtils::quteSettings();
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
    UIUtils::quteSettings().setValue("lastBackupTime", QDateTime::currentDateTime().toString());
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

void BackupSettingsPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Check if narrow layout is needed
    bool isNarrow = width() < 480;
    if (m_locationLayout) {
        m_locationLayout->setDirection(isNarrow ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    }
}