
#include "themesettingspage.h"
#include "thememanager.h"
#include "colorpicker.h"
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFormLayout>
#include <QApplication>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QFontComboBox>
#include <QVBoxLayout>

ThemeSettingsPage::ThemeSettingsPage(QWidget *parent)
    : QWidget(parent)
    , m_themeSelector(nullptr)
    , m_defaultFontCombo(nullptr)
    , m_menuFontCombo(nullptr)
    , m_defaultFontSize(nullptr)
    , m_zoomSlider(nullptr)
    , m_isUpdating(false)
{
    setupUI();
}

void ThemeSettingsPage::setupUI()
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(8,8,8,8);
    layout->setSpacing(8);

    m_themeSelector = new QComboBox(this);
    // Populate with available themes from ThemeManager
    const QStringList themes = ThemeManager::instance()->availableThemes();
    for (const QString &t : themes) m_themeSelector->addItem(t);
    connect(m_themeSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThemeSettingsPage::onThemeChanged);

    layout->addWidget(new QLabel("Theme:", this));
    layout->addWidget(m_themeSelector);

    createColorPickers();
    setupFontSection();
    setupZoomSection();

    // Export / Import
    auto btnLayout = new QHBoxLayout();
    auto exportBtn = new QPushButton("Export Theme", this);
    auto importBtn = new QPushButton("Import Theme", this);
    btnLayout->addWidget(exportBtn);
    btnLayout->addWidget(importBtn);
    layout->addLayout(btnLayout);

    connect(exportBtn, &QPushButton::clicked, this, &ThemeSettingsPage::onExportTheme);
    connect(importBtn, &QPushButton::clicked, this, &ThemeSettingsPage::onImportTheme);

    setLayout(layout);

    // Initialize previews to the current theme immediately
    loadSettings();

    // Keep the small color preview buttons in sync when the global theme changes
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](const Theme &) {
        loadSettings();
    });
}

void ThemeSettingsPage::createColorPickers()
{
    // Minimal set of color pickers exposed in the UI. The full theme supports many
    // roles; for now we expose a representative subset.
    QHBoxLayout *row = new QHBoxLayout();
    auto bgBtn = new QPushButton("Background", this);
    auto textBtn = new QPushButton("Text", this);
    auto menuBgBtn = new QPushButton("Toolbar BG", this);
    auto clickedBtn = new QPushButton("Clicked", this);
    row->addWidget(bgBtn);
    row->addWidget(textBtn);
    row->addWidget(menuBgBtn);
    row->addWidget(clickedBtn);
    if (auto mainLayout = qobject_cast<QVBoxLayout*>(this->layout())) mainLayout->addLayout(row);

    m_colorPickers.insert("background", bgBtn);
    m_colorPickers.insert("text", textBtn);
    m_colorPickers.insert("menuBackground", menuBgBtn);
    m_colorPickers.insert("clicked", clickedBtn);

    connect(bgBtn, &QPushButton::clicked, this, [this]() { onColorChanged("background", QColor()); });
    connect(textBtn, &QPushButton::clicked, this, [this]() { onColorChanged("text", QColor()); });
    connect(menuBgBtn, &QPushButton::clicked, this, [this]() { onColorChanged("menuBackground", QColor()); });
    connect(clickedBtn, &QPushButton::clicked, this, [this]() { onColorChanged("clicked", QColor()); });
}

void ThemeSettingsPage::setupFontSection()
{
    auto group = new QWidget(this);
    auto layout = new QFormLayout(group);
    
    // Editor font section
    m_defaultFontCombo = new QFontComboBox(group);
    m_defaultFontSize = new QSpinBox(group);
    m_defaultFontSize->setRange(6, 48);

    layout->addRow("Editor font:", m_defaultFontCombo);
    layout->addRow("Editor size:", m_defaultFontSize);

    connect(m_defaultFontCombo, &QFontComboBox::currentFontChanged, this, &ThemeSettingsPage::onFontChanged);
    connect(m_defaultFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeSettingsPage::onFontChanged);

    if (auto mainLayout = qobject_cast<QVBoxLayout*>(this->layout())) mainLayout->addWidget(group);
}

void ThemeSettingsPage::setupZoomSection()
{
    auto group = new QWidget(this);
    auto layout = new QFormLayout(group);
    
    // Zoom slider (100%, 150%, 200%)
    m_zoomSlider = new QSlider(Qt::Horizontal, group);
    m_zoomSlider->setRange(0, 2); // 3 positions: 0=100%, 1=150%, 2=200%
    m_zoomSlider->setValue(0);
    m_zoomSlider->setTickPosition(QSlider::TicksBelow);
    m_zoomSlider->setTickInterval(1);
    m_zoomSlider->setMinimumHeight(48); // Larger touch target
    m_zoomSlider->setStyleSheet("QSlider::handle:horizontal { width: 32px; height: 32px; margin: -8px 0; }");
    
    auto zoomLabel = new QLabel("100%", group);
    zoomLabel->setMinimumWidth(50); // Fixed width for label to prevent jumping
    
    // Update label while dragging (no theme application)
    connect(m_zoomSlider, &QSlider::valueChanged, this, [zoomLabel](int value) {
        const QStringList labels = {"100%", "150%", "200%"};
        if (value >= 0 && value < labels.size()) {
            zoomLabel->setText(labels[value]);
        }
    });
    
    // Apply zoom changes only when slider is released
    connect(m_zoomSlider, &QSlider::sliderReleased, this, &ThemeSettingsPage::onZoomChanged);
    
    auto zoomWidget = new QWidget(group);
    auto zoomLayout = new QHBoxLayout(zoomWidget);
    zoomLayout->setContentsMargins(0, 8, 0, 8); // Add vertical margins
    zoomLayout->addWidget(m_zoomSlider);
    zoomLayout->addWidget(zoomLabel);
    
    layout->addRow("UI Zoom:", zoomWidget);
    
    // Menu font section (below zoom)
    m_menuFontCombo = new QFontComboBox(group);
    layout->addRow("Menu font:", m_menuFontCombo);
    connect(m_menuFontCombo, &QFontComboBox::currentFontChanged, this, &ThemeSettingsPage::onFontChanged);

    if (auto mainLayout = qobject_cast<QVBoxLayout*>(this->layout())) mainLayout->addWidget(group);
}

void ThemeSettingsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_initialLoadDone) {
        loadSettings();
        m_initialLoadDone = true;
    }
}

void ThemeSettingsPage::loadSettings()
{
    m_isUpdating = true;
    Theme current = ThemeManager::instance()->currentTheme();
    m_currentTheme = current;

    // Select theme in combo if available
    int idx = m_themeSelector->findText(current.name);
    if (idx >= 0) m_themeSelector->setCurrentIndex(idx);

    // Apply basic colors to UI controls (if available)
    if (m_colorPickers.contains("background")) {
        QColor c = current.colors.background;
        if (c.isValid()) m_colorPickers["background"]->setStyleSheet(QString("background:%1").arg(c.name()));
    }
    if (m_colorPickers.contains("text")) {
        QColor c = current.colors.text;
        if (c.isValid()) m_colorPickers["text"]->setStyleSheet(QString("background:%1").arg(current.colors.text.name()));
    }
    if (m_colorPickers.contains("menuBackground")) {
        QColor c = current.colors.menuBackground;
        if (c.isValid()) m_colorPickers["menuBackground"]->setStyleSheet(QString("background:%1").arg(c.name()));
    }
    if (m_colorPickers.contains("clicked")) {
        QColor c = current.colors.clicked;
        if (c.isValid()) m_colorPickers["clicked"]->setStyleSheet(QString("background:%1").arg(c.name()));
    }

    // Fonts
    m_defaultFontCombo->setCurrentFont(current.defaultFont);
    m_defaultFontSize->setValue(current.defaultFont.pointSize() > 0 ? current.defaultFont.pointSize() : QApplication::font().pointSize());
    m_menuFontCombo->setCurrentFont(current.headerFont);

    // Zoom slider (calculate from touchTarget: 48=100%, 72=150%, 96=200%)
    int zoom = 0;
    if (current.metrics.touchTarget >= 96) zoom = 2;
    else if (current.metrics.touchTarget >= 72) zoom = 1;
    m_zoomSlider->setValue(zoom);

    m_isUpdating = false;
}

void ThemeSettingsPage::saveSettings()
{
    // Gather UI values into m_currentTheme and persist/apply
    if (m_isUpdating) return;

    m_currentTheme.name = m_themeSelector->currentText();
    m_currentTheme.defaultFont = m_defaultFontCombo->currentFont();
    m_currentTheme.defaultFont.setPointSize(m_defaultFontSize->value());
    m_currentTheme.headerFont = m_menuFontCombo->currentFont();

    // Calculate metrics from zoom slider
    int zoomValue = m_zoomSlider->value();
    const int baseTouch = 48;
    const int baseIcon = 24;
    const int baseSpacing = 8;
    const int baseBorder = 12;
    const int baseHeaderFontSize = 12; // Base header font size in points
    
    float multiplier = 1.0f;
    if (zoomValue == 1) multiplier = 1.5f;
    else if (zoomValue == 2) multiplier = 2.0f;
    
    m_currentTheme.metrics.touchTarget = static_cast<int>(baseTouch * multiplier);
    m_currentTheme.metrics.iconSize = static_cast<int>(baseIcon * multiplier);
    m_currentTheme.metrics.spacing = baseSpacing; // Keep spacing constant
    m_currentTheme.metrics.borderRadius = baseBorder; // Keep border radius constant
    
    // Scale header font size with zoom (keep user's font selection but scale the size)
    QFont headerFont = m_menuFontCombo->currentFont();
    headerFont.setPointSize(static_cast<int>(baseHeaderFontSize * multiplier));
    m_currentTheme.headerFont = headerFont;

    ThemeManager::instance()->saveTheme(m_currentTheme);
    
    // Apply theme using the proper applyTheme method like the dropdown does
    ThemeManager::instance()->applyTheme(m_currentTheme.name);
    
    emit settingsChanged();
}

void ThemeSettingsPage::onThemeChanged(int index)
{
    if (m_isUpdating) return;
    const QString name = m_themeSelector->itemText(index);
    ThemeManager::instance()->applyTheme(name);
    emit settingsChanged();
}

void ThemeSettingsPage::onColorChanged(const QString &role, const QColor &)
{
    // Open color picker and apply the chosen color to the theme manager.
    QColor initial = ThemeManager::instance()->themeColor(role);
    QColor c = ColorPicker::getColor(initial, this);
    if (c.isValid()) {
        ThemeManager::instance()->setThemeColor(role, c);
        // Update visual button if present
        if (m_colorPickers.contains(role)) {
            m_colorPickers[role]->setStyleSheet(QString("background:%1").arg(c.name()));
        }
        // Keep the local copy of the edited theme in sync so saveSettings() persists correctly
        if (role == "background") m_currentTheme.colors.background = c;
        else if (role == "text") m_currentTheme.colors.text = c;
        else if (role == "menuBackground") m_currentTheme.colors.menuBackground = c;
        else if (role == "clicked") m_currentTheme.colors.clicked = c;
        
        // Don't apply immediately, let user click Save & Apply
        emit settingsChanged();
    }
}

void ThemeSettingsPage::onFontChanged()
{
    if (m_isUpdating) return;
    // Update current theme fonts in memory only
    m_currentTheme.defaultFont = m_defaultFontCombo->currentFont();
    m_currentTheme.defaultFont.setPointSize(m_defaultFontSize->value());
    m_currentTheme.headerFont = m_menuFontCombo->currentFont();
}

void ThemeSettingsPage::onZoomChanged()
{
    if (m_isUpdating) return;
    
    // Calculate metrics from zoom slider and update in memory only
    int zoomValue = m_zoomSlider->value();
    const int baseTouch = 48;
    const int baseIcon = 24;
    const int baseSpacing = 8;
    const int baseBorder = 12;
    const int baseHeaderFontSize = 12; // Base header font size in points
    
    float multiplier = 1.0f;
    if (zoomValue == 1) multiplier = 1.5f;
    else if (zoomValue == 2) multiplier = 2.0f;
    
    m_currentTheme.metrics.touchTarget = static_cast<int>(baseTouch * multiplier);
    m_currentTheme.metrics.iconSize = static_cast<int>(baseIcon * multiplier);
    m_currentTheme.metrics.spacing = baseSpacing;
    m_currentTheme.metrics.borderRadius = baseBorder;
    
    // Scale header font size with zoom
    m_currentTheme.headerFont.setPointSize(static_cast<int>(baseHeaderFontSize * multiplier));
}

void ThemeSettingsPage::onExportTheme()
{
    QString file = QFileDialog::getSaveFileName(this, "Export Theme", QString(), "JSON Theme Files (*.json)");
    if (file.isEmpty()) return;
    // Delegate to ThemeManager where the real serialization lives
    ThemeManager::instance()->saveTheme(m_currentTheme);
    // For now saveTheme in ThemeManager should handle writing to disk if desired.
}

void ThemeSettingsPage::onImportTheme()
{
    QString file = QFileDialog::getOpenFileName(this, "Import Theme", QString(), "JSON Theme Files (*.json)");
    if (file.isEmpty()) return;
    // For minimal implementation we just notify that settings changed; a full
    // implementation would parse and import via ThemeManager.
    emit newThemeRequested();
}
