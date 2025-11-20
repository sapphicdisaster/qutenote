#ifndef BACKUPSETTINGSPAGE_H
#define BACKUPSETTINGSPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>

class BackupSettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit BackupSettingsPage(QWidget *parent = nullptr);
    void saveSettings();
    void loadSettings();

signals:
    void settingsChanged(); // Emitted when backup settings are changed

private slots:
    void onBackupNow();
    void onRestoreBackup();
    void onBrowseBackupLocation();
    void onAutoBackupChanged(int state);

private:
    void setupUI();
    bool createBackup(const QString &path);
    bool restoreFromBackup(const QString &path);

    QLineEdit *m_backupLocationEdit;
    QCheckBox *m_autoBackupCheck;
    QSpinBox *m_autoBackupInterval;
    QLabel *m_lastBackupLabel;
    QPushButton *m_backupNowBtn;
    QPushButton *m_restoreBtn;
    QPushButton *m_browseBtn;
};

#endif // BACKUPSETTINGSPAGE_H