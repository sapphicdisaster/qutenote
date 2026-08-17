#ifndef BACKUPSETTINGSPAGE_H
#define BACKUPSETTINGSPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include "smartpointers.h"

class BackupSettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit BackupSettingsPage(QWidget *parent = nullptr);
    void saveSettings();
    void loadSettings();

signals:
    void settingsChanged(); // Emitted when backup settings are changed

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onBackupNow();
    void onRestoreBackup();
    void onBrowseBackupLocation();
    void onAutoBackupChanged(int state);

private:
    void setupUI();
    bool createBackup(const QString &path);
    bool restoreFromBackup(const QString &path);

    QuteNote::OwnedPtr<QLineEdit> m_backupLocationEdit;
    QuteNote::OwnedPtr<QCheckBox> m_autoBackupCheck;
    QuteNote::OwnedPtr<QSpinBox> m_autoBackupInterval;
    QuteNote::OwnedPtr<QLabel> m_lastBackupLabel;
    QuteNote::OwnedPtr<QPushButton> m_backupNowBtn;
    QuteNote::OwnedPtr<QPushButton> m_restoreBtn;
    QuteNote::OwnedPtr<QPushButton> m_browseBtn;
    QuteNote::OwnedPtr<QHBoxLayout> m_locationLayout;
};

#endif // BACKUPSETTINGSPAGE_H