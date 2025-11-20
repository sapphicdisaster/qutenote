#include "settingsview.h"
#include "licensesettingspage.h"
#include "colorpicker.h"
#include "aboutdialog.h"
#include "thememanager.h"
#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QScrollBar>
#include <QFormLayout>

SettingsView::SettingsView(QWidget *parent)
    : ComponentBase(parent)
{
    // Settings will be initialized in setupComponent()
}

void SettingsView::initializeComponent()
{
    if (isInitialized()) {
        return;
    }
    
    setupComponent();
    markInitialized();
    emit componentInitialized();
}

SettingsView::~SettingsView()
{
    cleanupComponent();
}

void SettingsView::setupComponent()
{
    m_settings = QuteNote::makeOwned<QSettings>("QuteNote", "QuteNote");
    setupUI();
    loadSettings();
    
    // Connect to theme manager
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &SettingsView::onThemeChanged);

    // Connect touch handler signals
    if (m_touchHandler) {
        connect(m_touchHandler.get(), &SettingsViewTouchHandler::overscrollAmountChanged,
                this, &SettingsView::handleOverscroll);
    }

    // Manually apply the theme when the component is first set up
    onThemeChanged();
}

void SettingsView::cleanupComponent()
{
    // Block all signals during cleanup to prevent calling into destroyed parent objects
    blockSignals(true);
    
    // Make sure settings are saved before cleanup (won't emit signals now)
    saveSettings();
    
    // Components will be cleaned up automatically through OwnedPtr
}

void SettingsView::refreshComponent() 
{
    loadSettings();
}

void SettingsView::setupUI()
{
    // Set up touch interaction first
    m_touchHandler = QuteNote::makeOwned<SettingsViewTouchHandler>(this);

    m_tabWidget = QuteNote::makeOwned<QTabWidget>();

    setupGeneralTab();
    setupAppearanceTab();
    setupAdvancedTab();
    setupAboutTab();

    // Add tab widget to scroll area for touch interaction
    if (m_touchHandler && m_touchHandler->scrollArea()) {
        m_touchHandler->scrollArea()->setWidget(m_tabWidget.get());
        m_touchHandler->scrollArea()->setWidgetResizable(true);
    }

    m_mainLayout = QuteNote::makeOwned<QVBoxLayout>();
    setLayout(m_mainLayout.get());
    
    if (m_touchHandler && m_touchHandler->scrollArea()) {
        m_mainLayout->addWidget(m_touchHandler->scrollArea());
        m_touchHandler->scrollArea()->show();
    }

    // Add bottom bar with Back button on left and Save&Apply on right
    auto bottomBar = new QWidget();
    auto bottomBarLayout = new QHBoxLayout(bottomBar);
    bottomBarLayout->setContentsMargins(8, 8, 8, 8);
    
    auto backBtn = new QPushButton("Back to Main");
    backBtn->setMinimumHeight(48);
    bottomBarLayout->addWidget(backBtn);
    
    bottomBarLayout->addStretch();
    
    auto settingsBtn = new QPushButton("✓ Save && Apply");
    settingsBtn->setMinimumHeight(48);
    bottomBarLayout->addWidget(settingsBtn);
    
    bottomBar->setLayout(bottomBarLayout);
    m_mainLayout->addWidget(bottomBar);
    
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        saveSettings();
        emit settingsChanged();
    });
    
    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        saveSettings();
        emit settingsChanged();
    });
    
    // Ensure all widgets are visible
    if (m_tabWidget) m_tabWidget->show();
}

void SettingsView::setupGeneralTab()
{

    /**
     * Setup General tab layout using QFormLayout for accessibility and clarity.
     */
    m_generalTab = QuteNote::makeOwned<QWidget>();
    m_generalTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_notesDirLabel = QuteNote::makeOwned<QLabel>("Notes Directory:", m_generalTab.get());
    m_notesDirEdit = QuteNote::makeOwned<QLineEdit>(m_generalTab.get());
    m_browseDirBtn = QuteNote::makeOwned<QPushButton>("Browse...", m_generalTab.get());

    QWidget *notesDirWidget = new QWidget(m_generalTab.get());
    auto notesDirLayout = new QHBoxLayout(notesDirWidget);
    notesDirLayout->setContentsMargins(0,0,0,0);
    notesDirLayout->addWidget(m_notesDirEdit.get());
    notesDirLayout->addWidget(m_browseDirBtn.get());
    notesDirWidget->setLayout(notesDirLayout);

    m_languageLabel = QuteNote::makeOwned<QLabel>("Language:", m_generalTab.get());
    m_languageCombo = QuteNote::makeOwned<QComboBox>(m_generalTab.get());
    // Limit width so combo doesn't extend past edge on small screens
    m_languageCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_languageCombo->setMaximumWidth(320);
    // Use standard touch target height like other dropdowns
    int touchTarget = ThemeManager::instance()->currentTheme().metrics.touchTarget;
    if (touchTarget <= 0) touchTarget = 48;
    m_languageCombo->setMinimumHeight(touchTarget);
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem("Spanish", "es");
    m_languageCombo->addItem("French", "fr");
    m_languageCombo->addItem("German", "de");
    m_languageCombo->addItem("Chinese", "zh");

    m_autoSaveCheck = QuteNote::makeOwned<QCheckBox>("Enable Auto-save", m_generalTab.get());
    m_showSidebarCheck = QuteNote::makeOwned<QCheckBox>("Show Sidebar by Default", m_generalTab.get());

    auto formLayout = QuteNote::makeOwned<QFormLayout>(m_generalTab.get());
    formLayout->addRow(m_notesDirLabel.get(), notesDirWidget);
    formLayout->addRow(m_languageLabel.get(), m_languageCombo.get());
    formLayout->addRow(m_autoSaveCheck.get());
    formLayout->addRow(m_showSidebarCheck.get());
    formLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    m_generalTab->setLayout(formLayout.get());

    connect(m_browseDirBtn.get(), &QPushButton::clicked, this, &SettingsView::onBrowseNotesDirectory);
    connect(m_languageCombo.get(), &QComboBox::currentTextChanged, this, &SettingsView::settingsChanged);
    // Don't emit settingsChanged for checkboxes - they save immediately without closing settings
    connect(m_autoSaveCheck.get(), &QCheckBox::toggled, this, [this](bool checked) {
        m_settings->setValue("autoSave", checked);
    });
    connect(m_showSidebarCheck.get(), &QCheckBox::toggled, this, [this](bool checked) {
        m_settings->setValue("showSidebarByDefault", checked);
    });

    m_tabWidget->addTab(m_generalTab.get(), tr("General"));
}

void SettingsView::setupAppearanceTab()
{
    m_appearanceTab = QuteNote::makeOwned<QWidget>();
    m_appearanceTab->setObjectName("AppearanceTab");
    QScrollArea *scrollArea = new QScrollArea(m_appearanceTab.get());
    scrollArea->setWidgetResizable(true);
    QWidget *contentWidget = new QWidget(scrollArea);
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setSpacing(12);

    m_themeSettings = QuteNote::makeOwned<ThemeSettingsPage>(contentWidget);
    layout->addWidget(m_themeSettings.get());

    contentWidget->setLayout(layout);
    scrollArea->setWidget(contentWidget);
    QVBoxLayout *outerLayout = new QVBoxLayout(m_appearanceTab.get());
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);
    m_appearanceTab->setLayout(outerLayout);

    connect(m_themeSettings.get(), &ThemeSettingsPage::settingsChanged, this, &SettingsView::settingsChanged);
    m_tabWidget->addTab(m_appearanceTab.get(), tr("Appearance"));
}

void SettingsView::setupAdvancedTab()
{
    m_advancedTab = QuteNote::makeOwned<QWidget>();
    m_advancedTab->setObjectName("AdvancedTab");
    QScrollArea *scrollArea = new QScrollArea(m_advancedTab.get());
    scrollArea->setWidgetResizable(true);
    QWidget *contentWidget = new QWidget(scrollArea);
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setSpacing(12);

    // Backup settings page
    m_backupSettings = QuteNote::makeOwned<BackupSettingsPage>(contentWidget);
    layout->addWidget(m_backupSettings.get());

    // Reset button
    m_resetBtn = QuteNote::makeOwned<QPushButton>("Reset to Defaults", contentWidget);
    layout->addWidget(m_resetBtn.get());
    layout->addStretch(1);

    contentWidget->setLayout(layout);
    scrollArea->setWidget(contentWidget);
    QVBoxLayout *outerLayout = new QVBoxLayout(m_advancedTab.get());
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scrollArea);
    m_advancedTab->setLayout(outerLayout);

    connect(m_resetBtn.get(), &QPushButton::clicked, this, &SettingsView::onResetSettings);
    m_tabWidget->addTab(m_advancedTab.get(), tr("Advanced"));
}

void SettingsView::setupAboutTab()
{
    m_aboutTab = QuteNote::makeOwned<QWidget>();
    m_aboutTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // App info (centered)
    m_appNameLabel = QuteNote::makeOwned<QLabel>("QuteNote");
    QFont font = m_appNameLabel->font();
    font.setPointSize(18);
    font.setBold(true);
    m_appNameLabel->setFont(font);
    m_appNameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    m_versionLabel = QuteNote::makeOwned<QLabel>("Version 1.0.0");
    m_versionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // Info group
    // Short main about text taken from AboutDialog's main page
    m_descriptionLabel = QuteNote::makeOwned<QLabel>("qutenote is a simple but joyful note app\n"
                                   "it's built on the qt framework\n"
                                   "there are no ads and no hidden features, just notes\n"
                                   "it's forever free but you can support development with a donation");
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    // We no longer show the license summary here (licenses now live inside the About dialog)
    m_licenseLabel = QuteNote::makeOwned<QLabel>(QString());
    m_licenseLabel->setVisible(false);

    // Buttons (centered)
    m_aboutBtn = QuteNote::makeOwned<QPushButton>("License");
    m_donateBtn = QuteNote::makeOwned<QPushButton>("Donate");
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_aboutBtn.get());
    buttonLayout->addWidget(m_donateBtn.get());
    buttonLayout->addStretch();

    // Main layout (set directly on m_aboutTab)
    auto layout = new QVBoxLayout();
    layout->addSpacing(16);
    layout->addWidget(m_appNameLabel.get());
    layout->addWidget(m_versionLabel.get());
    layout->addSpacing(8);
    layout->addWidget(m_descriptionLabel.get());
    layout->addWidget(m_licenseLabel.get());
    layout->addSpacing(8);
    layout->addLayout(buttonLayout);
    layout->addStretch(1);
    m_aboutTab->setLayout(layout);

    connect(m_aboutBtn.get(), &QPushButton::clicked, this, &SettingsView::onAbout);
    // Donate logic moved here from AboutDialog
    connect(m_donateBtn.get(), &QPushButton::clicked, this, &SettingsView::onDonate);

    m_tabWidget->addTab(m_aboutTab.get(), "About");
}

void SettingsView::loadSettings()
{
    m_themeSettings->loadSettings();
    m_backupSettings->loadSettings();
    
    // Load notes directory setting
    QString notesDir = m_settings->value("notesDirectory", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QuteNote").toString();
    m_notesDirEdit->setText(notesDir);
    
    // Load auto-save and sidebar settings
    m_autoSaveCheck->setChecked(m_settings->value("autoSave", true).toBool());
    m_showSidebarCheck->setChecked(m_settings->value("showSidebarByDefault", true).toBool());
}

void SettingsView::saveSettings()
{
    m_themeSettings->saveSettings();
    m_backupSettings->saveSettings();
    
    // Save notes directory setting
    m_settings->setValue("notesDirectory", m_notesDirEdit->text());
    
    // Auto-save and sidebar settings are saved immediately when toggled, no need to save here
}

void SettingsView::onThemeChanged()
{
    // ThemeManager now handles the background color of the main view.
    
    // Apply theme to the tab widget using ThemeManager
    if (m_tabWidget) {
        ThemeManager::instance()->applyThemeToTabWidget(m_tabWidget.get());
    }

    // Ensure combo boxes in this view get themed as well
    if (m_languageCombo) {
        ThemeManager::instance()->applyThemeToComboBox(m_languageCombo.get());
        // Update minimum height to reflect current theme touch target when theme changes
        int touchTarget = ThemeManager::instance()->currentTheme().metrics.touchTarget;
        if (touchTarget <= 0) touchTarget = 48;
        m_languageCombo->setMinimumHeight(touchTarget);
    }

    // Ensure combos inside the custom pages/tabs are themed too
    if (m_themeSettings) {
        ThemeManager::instance()->applyThemeToComboBoxesInWidget(m_themeSettings.get());
    }
    // Also apply spinbox styling for controls inside the ThemeSettingsPage
    if (m_themeSettings) {
        ThemeManager::instance()->applyThemeToSpinBoxesInWidget(m_themeSettings.get());
    }
    if (m_backupSettings) {
        ThemeManager::instance()->applyThemeToComboBoxesInWidget(m_backupSettings.get());
        ThemeManager::instance()->applyThemeToSpinBoxesInWidget(m_backupSettings.get());
    }
    if (m_appearanceTab) {
        ThemeManager::instance()->applyThemeToComboBoxesInWidget(m_appearanceTab.get());
    }
    if (m_advancedTab) {
        ThemeManager::instance()->applyThemeToComboBoxesInWidget(m_advancedTab.get());
    }

    emit settingsChanged();
}

void SettingsView::onFontChanged()
{
    // This would typically open a font dialog
    // For now, we'll implement basic functionality
    bool ok;
    QFont font = QFontDialog::getFont(&ok, QApplication::font(), this, "Choose Editor Font");
    if (ok) {
        // Store font information
        m_settings->setValue("fontFamily", font.family());
        m_settings->setValue("fontSize", font.pointSize());
        emit settingsChanged();
    }
}

void SettingsView::onColorChanged()
{
    auto btn = qobject_cast<QPushButton*>(sender());
    QColor color;

    if (btn == m_editorColorBtn.get()) {
        color = ColorPicker::getColor(Qt::white, this);
        if (color.isValid()) {
            m_settings->setValue("editorBackgroundColor", color.name());
        }
    } else if (btn == m_sidebarColorBtn.get()) {
        color = ColorPicker::getColor(Qt::lightGray, this);
        if (color.isValid()) {
            m_settings->setValue("sidebarBackgroundColor", color.name());
        }
    }

    emit settingsChanged();
}

void SettingsView::onBrowseNotesDirectory()
{
    QString currentDir = m_notesDirEdit->text();
    if (currentDir.isEmpty()) {
        currentDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QuteNote";
    }

    QString dir = QFileDialog::getExistingDirectory(this, "Select Notes Directory",
                                                   currentDir,
                                                   QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_notesDirEdit->setText(dir);
    }
}

void SettingsView::onResetSettings()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Reset Settings",
        "Are you sure you want to reset all settings to defaults?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_settings->clear();
        loadSettings(); // Reload defaults
        emit settingsChanged();
    }
}

void SettingsView::onAbout()
{
    AboutDialog dlg(this);
    // Show as a modal fullscreen-friendly dialog
    dlg.exec();
}

void SettingsView::handleOverscroll(qreal amount)
{
    if (!m_touchHandler || !m_touchHandler->scrollArea() || !m_touchHandler->scrollArea()->verticalScrollBar())
        return;
        
    // Apply overscroll effect
    int scroll = m_touchHandler->scrollArea()->verticalScrollBar()->value();
    scroll -= amount; // Negative because overscroll amount is typically negative when pulling down
    m_touchHandler->scrollArea()->verticalScrollBar()->setValue(scroll);
}

bool SettingsView::eventFilter(QObject *watched, QEvent *event)
{
    // Default implementation - derived classes can override
    // to handle specific events
    return ComponentBase::eventFilter(watched, event);
}

void SettingsView::onDonate()
{
    // Open the donate link in an external browser (Android-friendly)
    QDesktopServices::openUrl(QUrl("https://ko-fi.com/411omen/tip"));
}

void SettingsView::setupLicenseTab()
{
    // License tab removed: license information is now available inside the About dialog.
}
