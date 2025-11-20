#ifndef SETTINGSVIEW_H
#define SETTINGSVIEW_H

#include "componentbase.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QColorDialog>
#include <QFontDialog>
#include <QListWidget>
#include <QTextEdit>
#include <QSettings>
#include "themesettingspage.h"
#include "backupsettingspage.h"
#include "licensesettingspage.h"
#include "settingsviewtouchhandler.h"

class SettingsView : public QuteNote::ComponentBase
{
    Q_OBJECT

public:
    explicit SettingsView(QWidget *parent = nullptr);
    ~SettingsView() override;

    void loadSettings();
    void saveSettings();

    // ComponentBase overrides
    void initializeComponent() override;
    void setupComponent() override;
    void cleanupComponent() override;
    void refreshComponent() override;

Q_SIGNALS:
    void settingsChanged();
    void backToMain();

private slots:
    void onThemeChanged();
    void onFontChanged();
    void onColorChanged();
    void onBrowseNotesDirectory();
    void onResetSettings();
    void onAbout();
    void onDonate();

private:
    void setupUI();
    void setupGeneralTab();
    void setupAppearanceTab();
    void setupAdvancedTab();
    void setupAboutTab();
    void setupLicenseTab();
    void handleOverscroll(qreal amount);
    
    SettingsViewTouchHandler* touchHandler() const { return m_touchHandler.get(); }
    
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    QuteNote::OwnedPtr<SettingsViewTouchHandler> m_touchHandler;
    QuteNote::OwnedPtr<QTabWidget> m_tabWidget;

    // General settings
    QuteNote::OwnedPtr<QWidget> m_generalTab;
    QuteNote::OwnedPtr<QLabel> m_notesDirLabel;
    QuteNote::OwnedPtr<QLineEdit> m_notesDirEdit;
    QuteNote::OwnedPtr<QPushButton> m_browseDirBtn;
    QuteNote::OwnedPtr<QLabel> m_languageLabel;
    QuteNote::OwnedPtr<QComboBox> m_languageCombo;
    QuteNote::OwnedPtr<QCheckBox> m_autoSaveCheck;
    QuteNote::OwnedPtr<QCheckBox> m_showSidebarCheck;

    // Appearance settings
    QuteNote::OwnedPtr<QWidget> m_appearanceTab;
    QuteNote::OwnedPtr<ThemeSettingsPage> m_themeSettings;
    QuteNote::OwnedPtr<BackupSettingsPage> m_backupSettings;
    QuteNote::OwnedPtr<QLabel> m_themeLabel;
    QuteNote::OwnedPtr<QComboBox> m_themeCombo;
    QuteNote::OwnedPtr<QLabel> m_fontLabel;
    QuteNote::OwnedPtr<QPushButton> m_fontBtn;
    QuteNote::OwnedPtr<QLabel> m_fontSizeLabel;
    QuteNote::OwnedPtr<QSpinBox> m_fontSizeSpin;
    QuteNote::OwnedPtr<QLabel> m_editorColorLabel;
    QuteNote::OwnedPtr<QPushButton> m_editorColorBtn;
    QuteNote::OwnedPtr<QLabel> m_sidebarColorLabel;
    QuteNote::OwnedPtr<QPushButton> m_sidebarColorBtn;

    // Advanced settings
    QuteNote::OwnedPtr<QWidget> m_advancedTab;
    QuteNote::OwnedPtr<QLabel> m_backupLabel;
    QuteNote::OwnedPtr<QCheckBox> m_backupCheck;
    QuteNote::OwnedPtr<QLabel> m_backupIntervalLabel;
    QuteNote::OwnedPtr<QSpinBox> m_backupIntervalSpin;
    QuteNote::OwnedPtr<QLabel> m_maxRecentLabel;
    QuteNote::OwnedPtr<QSpinBox> m_maxRecentSpin;
    QuteNote::OwnedPtr<QPushButton> m_resetBtn;

    // About tab
    QuteNote::OwnedPtr<QWidget> m_aboutTab;
    QuteNote::OwnedPtr<QLabel> m_appNameLabel;
    QuteNote::OwnedPtr<QLabel> m_versionLabel;
    QuteNote::OwnedPtr<QLabel> m_descriptionLabel;
    QuteNote::OwnedPtr<QLabel> m_licenseLabel;
    QuteNote::OwnedPtr<QPushButton> m_aboutBtn;
    QuteNote::OwnedPtr<QPushButton> m_donateBtn;

    // License tab
    QuteNote::OwnedPtr<LicenseSettingsPage> m_licenseTab;

    QuteNote::OwnedPtr<QVBoxLayout> m_mainLayout;
    QuteNote::OwnedPtr<QSettings> m_settings;
};

#endif // SETTINGSVIEW_H
