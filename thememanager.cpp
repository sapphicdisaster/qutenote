// Includes (moved to top for proper identifier resolution)
#include "thememanager.h"
#include "texteditor.h"
#include "mainview.h"
#include "settingsview.h"
#include <QMap>
#include <QStringList>
#include <QString>
#include <QColor>

void ThemeManager::setThemeColor(const QString &role, const QColor &color)
{
    // Map role to ThemeColors field
    if (role == "primary") m_currentTheme.colors.primary = color;
    else if (role == "secondary") m_currentTheme.colors.secondary = color;
    else if (role == "background") m_currentTheme.colors.background = color;
    else if (role == "surface") m_currentTheme.colors.surface = color;
    else if (role == "text") m_currentTheme.colors.text = color;
    else if (role == "textSecondary") m_currentTheme.colors.textSecondary = color;
    else if (role == "accent") m_currentTheme.colors.accent = color;
    else if (role == "menuBackground") m_currentTheme.colors.menuBackground = color;
    else if (role == "editorMenuBackground") m_currentTheme.colors.editorMenuBackground = color;
    else if (role == "clicked") m_currentTheme.colors.clicked = color;
    else if (role == "toolbarTextIcon") m_currentTheme.colors.toolbarTextIcon = color;
    else if (role == "border") m_currentTheme.colors.border = color;
    else if (role == "error") m_currentTheme.colors.error = color;
    else if (role == "success") m_currentTheme.colors.success = color;
    else return;
    emit themeChanged(m_currentTheme);
}

QColor ThemeManager::themeColor(const QString &role) const
{
    if (role == "primary") return m_currentTheme.colors.primary;
    if (role == "secondary") return m_currentTheme.colors.secondary;
    if (role == "background") return m_currentTheme.colors.background;
    if (role == "surface") return m_currentTheme.colors.surface;
    if (role == "text") return m_currentTheme.colors.text;
    if (role == "textSecondary") return m_currentTheme.colors.textSecondary;
    if (role == "accent") return m_currentTheme.colors.accent;
    if (role == "menuBackground") return m_currentTheme.colors.menuBackground;
    if (role == "editorMenuBackground") return m_currentTheme.colors.editorMenuBackground;
    if (role == "clicked") return m_currentTheme.colors.clicked;
    if (role == "toolbarTextIcon") return m_currentTheme.colors.toolbarTextIcon;
    if (role == "border") return m_currentTheme.colors.border;
    if (role == "error") return m_currentTheme.colors.error;
    if (role == "success") return m_currentTheme.colors.success;
    return QColor();
}

#include "filebrowser.h"
#include <QApplication>
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QSize>
#include <QSplitter>
#include <QComboBox>
#include <QWidget>
#include <QSpinBox>

// Singleton implementation now handled by QuteNote::Singleton

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    initializeDefaultThemes();
    applyCurrentThemeStyles();
}

void ThemeManager::applyCurrentThemeStyles()
{
    // Connect to theme changes to update component themes
    // Use explicit tokens and replace them to avoid any risk of placeholder/arg
    // count mismatches that cause runtime 'QString::arg: Argument missing' warnings.
    QString styleSheet = QString(R"(
        QMainWindow {
            background-color: {{BACKGROUND}};
        }
        MainView, SettingsView, ThemeSettingsPage, BackupSettingsPage {
        }
        QMessageBox {
            border-radius: {{BORDER_RADIUS}}px;
            border: 2px solid {{BORDER}};
        }
        QMessageBox QPushButton {
            background-color: {{PLATE}};
            color: white;
        }
        /* Removed global QWidget font-family and font-size to prevent layout issues */
        QPushButton {
            min-height: {{TOUCH_TARGET}}px;
            padding: {{SPACING}}px {{SPACING2}}px;
            border-radius: {{BORDER_RADIUS}}px;
            /* Use surface color for generic button backgrounds so toolbar/menu plates
               defined elsewhere (menuBackground) remain authoritative. This uses
               ThemeManager values rather than hard-coded colors. */
            background: {{PLATE}};
            border: 1px solid {{BORDER}};
        }
        QPushButton:hover {
            background: {{SECONDARY}};
        }
        /* Toolbar background and checked / active state for checkable buttons (toolbar toggles, etc.) */
             /* Make toolbars use the derived plate color so the toolbar buttons
                 region matches the fixed plate areas and scrollbar tracks. */
             QToolBar { background: {{PLATE}}; }
             /* Ensure any scrollbars inside toolbars use the same plate color for
                 their tracks/grooves so the area behind the handle doesn't appear
                 lighter than the toolbar plate. */
             QToolBar QScrollBar:horizontal, QToolBar QScrollBar:vertical { background: {{PLATE}}; }
             QToolBar QScrollBar::groove:horizontal, QToolBar QScrollBar::groove:vertical { background: {{PLATE}}; }
                     /* Specific targets for toolbar plate areas (fixed left/right and scroll area viewport)
                        use a slightly darker plate color so fixed components read equal or darker than
                        toolbar buttons. These are targeted by objectName set on MainView widgets. */
                    #ToolbarRow, #ToolbarLeftFixed, #ToolbarArea, #ToolbarArea QWidget, #FileBrowserButtonContainer { background: {{PLATE}}; }        QToolButton:checked, QPushButton:checked {
            background: {{CHECKED_BORDER}};
            color: {{CHECKED_TEXT}};
            border: 1px solid {{MENUBG}};
        }
        QToolButton:checked:hover, QPushButton:checked:hover {
            background: {{PLATE}};
        }
        /* Set base for text entry widgets */
        QTextEdit, QLineEdit, QPlainTextEdit {
            background-color: {{BASE}};
            border: none;
            padding: 0px;
            margin: 0px;
            selection-background-color: {{SELECT_BG}};
            selection-color: {{SELECT_TEXT}};
            font-size: 16px;
        }
        /* Specific targets for toolbar plate areas (fixed left/right and scroll area viewport)
           use a slightly darker plate color so fixed components read equal or darker than
           toolbar buttons. These are targeted by objectName set on MainView widgets. */
        #ToolbarRow, #ToolbarLeftFixed, #ToolbarArea > .qt_scrollarea_viewport, #FileBrowserButtonContainer {
            background: {{PLATE}};
        }

        QScrollBar:vertical, QScrollBar:horizontal {
            /* Use the toolbar plate color for scrollbar tracks in toolbar areas so the
               area behind the handle doesn't appear lighter than the toolbar plate. */
            background: {{PLATE}};
            border-radius: 6px;
            width: 12px;
            height: 12px;
            margin: 0px;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: {{ACCENT}};
            min-height: 24px;
            min-width: 24px;
            border-radius: 6px;
            border: 1px solid {{BORDER}};
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            background: none;
            border: none;
        }
        QScrollBar::up-arrow, QScrollBar::down-arrow,
        QScrollBar::left-arrow, QScrollBar::right-arrow {
            background: none;
            border: none;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: none;
        }

        QSlider::groove:horizontal {
            border: 1px solid {{SLIDER_BORDER}};
            height: 8px;
            background: {{SLIDER_SURFACE}};
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            background: {{SLIDER_BACKGROUND}};
            border: 1px solid {{SLIDER_BORDER}};
            width: 22px;
            height: 22px;
            margin: -7px 0;
            border-radius: 11px;
        }
        QSlider::groove:vertical {
            border: 1px solid {{SLIDER_BORDER}};
            width: 8px;
            background: {{SLIDER_SURFACE}};
            border-radius: 4px;
        }
        QSlider::handle:vertical {
            background: {{SLIDER_BACKGROUND}};
            border: 1px solid {{SLIDER_BORDER}};
            width: 22px;
            height: 22px;
            margin: 0 -7px;
            border-radius: 11px;
        }
        {{SWITCH_STYLE}}
    )");

    // Ensure we have the current theme and derived colors available before
    // performing token replacements. This prevents use-before-declaration
    // build errors and keeps all theme logic in one place.
    Theme theme = m_currentTheme;
    QColor checkedBg = theme.colors.clicked.isValid() ? theme.colors.clicked : theme.colors.accent.darker(115);
    QColor checkedHover = theme.colors.clicked.isValid() ? theme.colors.clicked.darker(110) : theme.colors.accent.darker(125);
    QString plateColor = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.darker(110).name()
        : theme.colors.background.darker(130).name();

    // Replace tokens with actual theme values explicitly (order-independent)
    styleSheet.replace("{{TOUCH_TARGET}}", QString::number(theme.metrics.touchTarget));
    styleSheet.replace("{{SPACING}}", QString::number(theme.metrics.spacing));
    styleSheet.replace("{{SPACING2}}", QString::number(theme.metrics.spacing * 2));
    styleSheet.replace("{{BORDER_RADIUS}}", QString::number(theme.metrics.borderRadius));
    styleSheet.replace("{{HIGHLIGHT}}", theme.colors.highlight.name());
    styleSheet.replace("{{PRIMARY}}", theme.colors.primary.name());
    styleSheet.replace("{{BORDER}}", theme.colors.border.name());
    styleSheet.replace("{{SECONDARY}}", theme.colors.secondary.name());
    styleSheet.replace("{{SURFACE}}", theme.colors.surface.name());
    styleSheet.replace("{{ACCENT}}", theme.colors.accent.name());
    styleSheet.replace("{{BACKGROUND}}", theme.colors.background.name());
    styleSheet.replace("{{SLIDER_BORDER}}", theme.colors.border.name());
    styleSheet.replace("{{SLIDER_SURFACE}}", theme.colors.surface.name());
    styleSheet.replace("{{SLIDER_BACKGROUND}}", theme.colors.background.name());
    styleSheet.replace("{{CHECKED_BORDER}}", checkedBg.name());
    styleSheet.replace("{{CHECKED_TEXT}}", theme.colors.surface.name());
    styleSheet.replace("{{CHECKED_HOVER}}", checkedHover.name());
    styleSheet.replace("{{MENUBG}}", theme.colors.menuBackground.name());
    styleSheet.replace("{{PLATE}}", plateColor);
    styleSheet.replace("{{BASE}}", theme.colors.base.name());
    styleSheet.replace("{{SWITCH_STYLE}}", theme.switchStyle);
    QApplication::setFont(theme.defaultFont);

    // Apply the main stylesheet now (token replacements completed)
    qApp->setStyleSheet(styleSheet);

    // Additional targeted overrides for unified UI styling that shouldn't
    // change theme colors. These are kept separate to avoid disturbing the
    // large positional-arg string above.
    // Compute derived colors used for outlines and combo backgrounds so they
    // remain tied to the theme without hard-coding color values.
    QString outlineColor = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.darker(140).name()
        : theme.colors.border.darker(140).name();
    QString comboBg = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.name()
        : theme.colors.surface.name();
    QString comboText = theme.colors.toolbarTextIcon.isValid()
        ? theme.colors.toolbarTextIcon.name()
        : theme.colors.text.name();

    QString extra = QString(R"(
          /* FileBrowser buttons and toolbar buttons: bolder outline, no shadow, full surround */
          /* Also include toolbar fixed-area buttons so toggle/settings in MainView
              pick up the same plate/background without needing per-widget styles. */
          #FileBrowserButtonContainer QPushButton, QToolBar QToolButton, TitleBarWidget QToolButton,
          #ToolbarLeftFixed QToolButton, #ToolbarRow QToolButton {
            background: {{COMBO_BG}};
            color: {{COMBO_TEXT}};
            border: 2px solid {{OUTLINE_COLOR}};
            /* box-shadow is not supported by Qt stylesheets; skip to avoid warnings */
            border-top-left-radius: {{BORDER_RADIUS}}px;
            border-top-right-radius: {{BORDER_RADIUS}}px;
            border-bottom-left-radius: {{BORDER_RADIUS}}px;
            border-bottom-right-radius: {{BORDER_RADIUS}}px;
            padding: {{SPACING}}px {{SPACING}}px;
            min-height: 34px;
        }

        /* Apply same outer outline to the TitleBarWidget itself so the titlebar
           region matches the button outlines visually. Only the border is set
           so we don't alter the titlebar's background color. */
        TitleBarWidget {
            border: 2px solid {{OUTLINE_COLOR}};
            border-radius: {{BORDER_RADIUS}}px;
        }

        /* Font selector and size combo in TextEditor: curved corners, no drop shadow.
           Force a dark combo background, white text, and make the drop button match
           the dark background with a thicker outline like the other toolbar icons. */
        QComboBox#fontComboBox, QComboBox#fontSizeComboBox {
            border-radius: {{BORDER_RADIUS}}px;
            /* use outlineColor for the overall combo border but hide its right edge so
               the drop area can present a stronger/thicker edge without a double seam */
            border: 2px solid {{OUTLINE_COLOR}}; /* increased by 1px as requested */
            border-right: none;
            background: {{COMBO_BG}};
            color: {{COMBO_TEXT}};
            padding: {{SPACING}}px;
            /* box-shadow is not supported by Qt stylesheets; skip to avoid warnings */
        }
        QComboBox#fontComboBox::drop-down, QComboBox#fontSizeComboBox::drop-down {
            /* The drop area should match the combo background and present a thicker
               left edge so it reads like a distinct button; keep same dark bg. */
            border-left: 2px solid {{OUTLINE_COLOR}};
            background: {{COMBO_BG}};
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 30px;
            border-top-right-radius: {{BORDER_RADIUS}}px;
            border-bottom-right-radius: {{BORDER_RADIUS}}px;
            /* box-shadow is not supported by Qt stylesheets; skip to avoid warnings */
        }
        QComboBox#fontComboBox::down-arrow, QComboBox#fontSizeComboBox::down-arrow {
            /* Use a bundled SVG resource for the arrow so it is always visible
               on all platforms and doesn't require URL-escaping. */
            image: url(:/resources/icons/custom/chevron-down.svg);
            width: 10px;
            height: 6px;
            border: none;
        }

        /* Toolbar dividers should be subtle and driven by the theme border color */
        QToolBar::separator {
            background: {{OUTLINE_COLOR}};
            width: 2px;
            margin: 0 8px;
            min-height: 24px;
            border-radius: 1px;
        }

        /* Status bar should match the top bar plate and use white text */
        QStatusBar {
            background: {{PLATE_COLOR}};
            color: #ffffff;
        }
        QStatusBar QLabel, QStatusBar * { color: #ffffff; }
        QWidget[touch-friendly=true]:pressed { background-color: rgba(128,128,128,0.12); }

        /* Global QComboBox dropdown styling so all combobox popups get themed */
        QComboBox QAbstractItemView {
            background: {{COMBO_BG}};
            color: {{COMBO_TEXT}};
            selection-background-color: {{ACCENT}};
            selection-color: {{SURFACE}};
            border: 1px solid {{OUTLINE_COLOR}};
        }
        )");

    extra.replace("{{OUTLINE_COLOR}}", outlineColor);
    extra.replace("{{BORDER_RADIUS}}", QString::number(theme.metrics.borderRadius));
    extra.replace("{{SPACING}}", QString::number(theme.metrics.spacing));
    extra.replace("{{COMBO_BG}}", comboBg);
    extra.replace("{{COMBO_TEXT}}", comboText);
    extra.replace("{{PLATE_COLOR}}", plateColor);
    extra.replace("{{ACCENT}}", theme.colors.accent.name());
    extra.replace("{{SURFACE}}", theme.colors.surface.name());

    // Combine the stylesheets and apply them to the application
    qApp->setStyleSheet(styleSheet + extra);
}

void ThemeManager::applyThemeToFileBrowser(FileBrowser *fileBrowser)
{
    if (!fileBrowser) return;

    Theme theme = currentTheme();
    if (auto *treeWidget = fileBrowser->treeWidget()) {
        QString styleSheet = QString(
            "QTreeWidget {"
            "  background-color: %1;"
            "  border: 2px solid %2;"
            "  border-top-left-radius: 8px;"
            "  border-top-right-radius: 8px;"
            "  border-bottom-left-radius: 8px;"
            "  border-bottom-right-radius: 8px;"
            "}"
            "QTreeWidget::item:selected {"
            "  background-color: %3;"
            "  color: %4;"
            "}"
        ).arg(theme.colors.base.name())
         .arg(theme.colors.highlight.name())
         .arg(theme.colors.accent.name())
         .arg(theme.colors.surface.name());
        treeWidget->setStyleSheet(styleSheet);
    }
}

void ThemeManager::applyThemeToSplitter(QSplitter *splitter)
{
    if (!splitter) return;

    Theme theme = currentTheme();
    QString styleSheet = QString(
        "QSplitter::handle {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "}"
        "QSplitter::handle:hover {"
        "  background: %3;"
        "}"
    ).arg(theme.colors.secondary.name()).arg(theme.colors.border.name()).arg(theme.colors.primary.name());
    splitter->setStyleSheet(styleSheet);
}

void ThemeManager::applyThemeToComboBox(QComboBox *combo)
{
    if (!combo) return;

    Theme theme = currentTheme();

    QString outlineColor = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.darker(140).name()
        : theme.colors.border.darker(140).name();
    QString comboBg = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.name()
        : theme.colors.surface.name();
    QString comboText = theme.colors.toolbarTextIcon.isValid()
        ? theme.colors.toolbarTextIcon.name()
        : theme.colors.text.name();

    QString styleSheet = QString(
        "QComboBox {"
        "  background: %1;"
        "  color: %2;"
        "  border: 2px solid %3;"
        "  border-radius: %4px;"
        "  padding: %5px;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 30px;"
        "  border-left: 2px solid %3;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/resources/icons/custom/chevron-down.svg);"
        "  width: 10px;"
        "  height: 6px;"
        "}"
    ).arg(comboBg).arg(comboText).arg(outlineColor).arg(theme.metrics.borderRadius).arg(theme.metrics.spacing);

    // Ensure the popup list uses the same background and text colors so items are readable
    QString popupSs = QString(
        "QComboBox QAbstractItemView {"
        "  background: %1;"
        "  color: %2;"
        "  selection-background-color: %3;"
        "  selection-color: %4;"
        "  border: 1px solid %5;"
        "}"
    ).arg(comboBg)
     .arg(comboText)
     .arg(theme.colors.accent.name())
     .arg(theme.colors.surface.name())
     .arg(outlineColor);

    styleSheet += popupSs;

    combo->setStyleSheet(styleSheet);
}

void ThemeManager::applyThemeToComboBoxesInWidget(QWidget *parent)
{
    if (!parent) return;
    auto combos = parent->findChildren<QComboBox*>();
    for (QComboBox *c : combos) {
        applyThemeToComboBox(c);
    }
}

void ThemeManager::applyThemeToSpinBox(QSpinBox *spin)
{
    if (!spin) return;

    Theme theme = currentTheme();

    QString outlineColor = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.darker(140).name()
        : theme.colors.border.darker(140).name();
    QString bg = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.name()
        : theme.colors.surface.name();
    QString text = theme.colors.toolbarTextIcon.isValid()
        ? theme.colors.toolbarTextIcon.name()
        : theme.colors.text.name();

    QString styleSheet = QString(
        "QSpinBox, QDoubleSpinBox {"
        "  background: %1;"
        "  color: %2;"
        "  border: 2px solid %3;"
        "  border-radius: %4px;"
        "  padding: %5px;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "  subcontrol-origin: border;"
        "  width: 30px;"
        "}"
    ).arg(bg).arg(text).arg(outlineColor).arg(theme.metrics.borderRadius).arg(theme.metrics.spacing);

    spin->setStyleSheet(styleSheet);
}

void ThemeManager::applyThemeToSpinBoxesInWidget(QWidget *parent)
{
    if (!parent) return;
    auto spins = parent->findChildren<QSpinBox*>();
    for (QSpinBox *s : spins) {
        applyThemeToSpinBox(s);
    }
}

void ThemeManager::applyThemeToTabWidget(QTabWidget *tabWidget)
{
    if (!tabWidget) return;

    Theme theme = currentTheme();
    // Calculate plateColor here
    QString plateColor = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.darker(110).name()
        : theme.colors.background.darker(130).name();

    QString styleSheet = QString(
        "QTabWidget::pane {" // The tab widget frame
        "    border-top: 1px solid %1;"
        "    background: %2;"
        "}"
        "QTabWidget::tab-bar {"
        "    left: 0px;" // Align tab bar to the left
        "}"
        "QTabBar::tab {"
        "    background: %3;"
        "    color: %4;"
        "    min-height: %5px;" // Set min-height to touchTarget
        "    padding: 5px 10px;"
        "    border: 1px solid %1;"
        "    border-bottom-color: %1;" // Same as pane border
        "    border-top-left-radius: %6px;"
        "    border-top-right-radius: %6px;"
        "    margin-right: 1px;"
        "}"
        "QTabBar::tab:selected {"
        "    background: %2;"
        "    color: %7;" // Use plateColor for selected tab text
        "    border-bottom-color: %2;" // Make border disappear for selected tab
        "}"
        "QTabBar::tab:hover {"
        "    background: %8;"
        "}"
    ).arg(theme.colors.border.name()) // %1
     .arg(theme.colors.background.name()) // %2
     .arg(theme.colors.surface.name()) // %3
     .arg(theme.colors.text.name()) // %4
     .arg(theme.metrics.touchTarget) // %5
     .arg(theme.metrics.borderRadius) // %6
     .arg(plateColor) // %7
     .arg(theme.colors.secondary.name()); // %8

    tabWidget->setStyleSheet(styleSheet);
}

QStringList ThemeManager::availableThemes() const
{
    return m_themes.keys();
}
Theme ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

Theme ThemeManager::loadTheme(const QString &themeName) const
{
    return m_themes.value(themeName);
}

void ThemeManager::saveTheme(const Theme &theme)
{
    m_themes.insert(theme.name, theme);
    
    // If we're saving the currently active theme, update m_currentTheme
    if (theme.name == m_currentTheme.name) {
        m_currentTheme = theme;
    }
    
    // Save to file system if needed
    QString path = themeFilePath(theme.name);
    QDir().mkpath(QFileInfo(path).path());
    
    QJsonObject themeObj;
    // Convert theme to JSON and save
    // ... (implementation omitted for brevity)
}

QString ThemeManager::themeFilePath(const QString &themeName) const
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QString("%1/themes/%2.json").arg(dataPath, themeName);
}

void ThemeManager::createPurpleTheme()
{
    Theme purple;
    purple.name = "Purple";
    purple.displayName = tr("Kuromi");
    purple.isDark = true;

    // Kuromi inspired theme (dark purple/black aesthetic)
    purple.colors.primary = QColor("#BD95E4");      // Purple for buttons
    purple.colors.secondary = QColor("#8A2BE2");    // Blue Violet
    purple.colors.background = QColor("#1A1A1D");   // Very Dark Gray (almost black)
    purple.colors.surface = QColor("#2C2C30");      // Dark Gray
    purple.colors.base = QColor("#3A3A3E");         // Slightly lighter Dark Gray for editor bg
    purple.colors.text = QColor("#FFFFFF");         // White for editor text
    purple.colors.textSecondary = QColor("#B0B0B0"); // Light Gray
    purple.colors.accent = QColor("#E0BBE4");       // Light Purple (for highlights)
    purple.colors.menuBackground = QColor("#BD95E4"); // Purple for menu/toolbar background
    purple.colors.clicked = QColor("#E6CFF5");        // Light purple for clicked states
    purple.colors.border = QColor("#6A0DAD");       // Darker Purple (for borders)
    purple.colors.error = QColor("#FF6B6B");        // Soft red
    purple.colors.success = QColor("#98FB98");      // Pale green
    purple.colors.toolbarTextIcon = QColor("#FFFFFF"); // White for icons
    purple.colors.highlight = QColor("#8A2BE2");    // Blue Violet (for selection/highlight)

    // Metrics
    purple.metrics.spacing = 8;
    purple.metrics.borderRadius = 12;
    purple.metrics.iconSize = 24;
    purple.metrics.touchTarget = 48;

    purple.defaultFont = QFont("Nunito Sans");
    purple.headerFont = QFont("Nunito Sans", 12); // Standardized to 12pt base

    purple.switchStyle = R"(
        QCheckBox::indicator {
            width: 40px;
            height: 24px;
            border-radius: 12px;
            border: 2px solid #8A2BE2; /* Blue Violet */
        }
        QCheckBox::indicator:unchecked {
            background: #2C2C30; /* Dark Gray */
        }
        QCheckBox::indicator:checked {
            background: #E0BBE4; /* Light Purple */
            border: 2px solid #5D3FD3; /* Dark Purple */
        }
    )";

    m_themes.insert(purple.name, purple);
}

void ThemeManager::createPinkTheme()
{
    Theme Pink;
    Pink.name = "Pink";
    Pink.displayName = tr("My Mallow");
    Pink.isDark = false;

    // My Mallow
    Pink.colors.primary = QColor("#FFC0CB");      // Pink
    Pink.colors.secondary = QColor("#FFB6C1");    // LightPink
    Pink.colors.background = QColor("#FFC0CB");   // Pink
    Pink.colors.surface = QColor("#FFF0F5");      // LavenderBlush
    Pink.colors.base = QColor("#FFEFF4");
    Pink.colors.text = QColor("#4A4A4A");        // Dark gray
    Pink.colors.textSecondary = QColor("#717171");// Medium gray
    Pink.colors.accent = QColor("#FF69B4");       // Hot pink
    // Companion UI colors for toolbar/menu and clicked button
    Pink.colors.menuBackground = QColor("#D4546A"); // Dark red-pink companion
    Pink.colors.clicked = QColor("#F8C8DC");        // Pink for clicked/toggled buttons
    Pink.colors.border = QColor("#FFE4E1");       // Misty rose
    Pink.colors.error = QColor("#FF6B6B");        // Soft red
    Pink.colors.success = QColor("#98FB98");      // Pale green
    Pink.colors.toolbarTextIcon = QColor("#FFFFFF");
    Pink.colors.highlight = QColor("#FFB6C1");

    // Metrics
    Pink.metrics.spacing = 8;
    Pink.metrics.borderRadius = 12;
    Pink.metrics.iconSize = 24;
    Pink.metrics.touchTarget = 48;

    Pink.defaultFont = QFont("Nunito Sans");
    Pink.headerFont = QFont("Nunito Sans", 12); // Standardized to 12pt base

    Pink.switchStyle = R"(
        QCheckBox::indicator {
            width: 40px;
            height: 24px;
            border-radius: 12px;
            border: 2px solid #FFB6C1;
        }
        QCheckBox::indicator:unchecked {
            background: #FFEFF4;
        }
        QCheckBox::indicator:checked {
            background: #FF69B4;
            border: 2px solid #FFC0CB;
        }
    )";

    m_themes.insert(Pink.name, Pink);
}

// Minimal default-theme initialization so the ThemeManager has at least one
// usable theme at startup. Keep this lightweight to avoid pulling in heavy
// dependencies here; UI components listen to the `themeChanged` signal.
void ThemeManager::initializeDefaultThemes()
{
    createPinkTheme();
    createPurpleTheme(); // Add the new Purple theme

    // Set Pink as the initial current theme
    m_currentTheme = m_themes.value("Pink");
}

// Apply a named theme by looking it up in the map and delegating to
// applyThemeToApplication. This keeps the public API small and predictable.
void ThemeManager::applyTheme(const QString &themeName)
{
    if (!m_themes.contains(themeName)) return;

    // Notify listeners that a theme application is starting. We then schedule
    // the actual work via a short singleShot so the UI has an opportunity to
    // render any loading overlay before performing heavier string/stylesheet
    // construction and application which may block briefly.
    emit themeApplyStarted();

    const QString tn = themeName;
    QTimer::singleShot(25, this, [this, tn]() {
        Theme t = m_themes.value(tn);
        applyThemeToApplication(t);
        applyCurrentThemeStyles();
        emit themeApplyFinished();
    });
}

// Lightweight implementation: update the current theme and emit change
// notifications. More detailed application-wide styling is performed where
// the theme is consumed (e.g., in the constructor or other UI code).
void ThemeManager::applyThemeToApplication(const Theme &theme)
{
    m_currentTheme = theme;
    emit themeChanged(m_currentTheme);
}

EditorTheme ThemeManager::editorTheme() const
{
    EditorTheme theme;
    theme.textColor = m_currentTheme.colors.text;
    theme.backgroundColor = m_currentTheme.colors.surface;
    theme.selectionColor = m_currentTheme.colors.surface;
    theme.selectionBackground = m_currentTheme.colors.accent;
    theme.lineNumberColor = m_currentTheme.colors.textSecondary;
    theme.lineNumberBackground = m_currentTheme.colors.background;
    theme.currentLineColor = m_currentTheme.colors.border;
    theme.editorFont = m_currentTheme.defaultFont;
    theme.fontSize = m_currentTheme.defaultFont.pointSize();
    return theme;
}

void ThemeManager::applyThemeToEditor(TextEditor *editor, const EditorTheme &theme)
{
    if (!editor) return;
    
    // Create and apply stylesheet for the editor
    QString styleSheet = QString(R"(
        QTextEdit {
            color: %1;
            background-color: %2;
            font-family: %3;
            font-size: %4pt;
            selection-color: %5;
            selection-background-color: %6;
        }
    )")
    .arg(theme.textColor.name())
    .arg(theme.backgroundColor.name())
    .arg(theme.editorFont.family())
    .arg(theme.fontSize)
    .arg(theme.selectionColor.name())
    .arg(theme.selectionBackground.name());
    
        // Apply editor stylesheet first
        if (editor) {
                // Determine toolbar background for the editor: prefer explicit editorMenuBackground
                // falling back to a darker variant of the general menuBackground.
                Theme cur = m_currentTheme;
                QColor toolbarBg = cur.colors.editorMenuBackground.isValid()
                    ? cur.colors.editorMenuBackground
                    : cur.colors.menuBackground.darker(120);
                QColor toolbarText = cur.colors.toolbarTextIcon.isValid()
                    ? cur.colors.toolbarTextIcon
                    : ((toolbarBg.lightness() < 128) ? QColor("#ffffff") : cur.colors.text);

                QString editorSs = styleSheet + QString("\n"
                    "TextEditor {"
                    "    background-color: %1;"
                    "    border: 2px solid %2;"
                    "    border-radius: %3px;"
                    "    padding: %4px;"
                    "}"
                    "QToolBar {"
                    "    spacing: 2px;"
                    "    padding: 0px;"
                    "    background: %5;"
                    "    color: %6;"
                    "}"
                    "QToolBar QToolButton {"
                    "    min-width: 48px;"
                    "    min-height: 44px;"
                    "    padding: 4px;"
                    "    margin: 1px;"
                    "    border: 2px solid %2;"
                    "    border-top-left-radius: %3px;"
                    "    border-top-right-radius: %3px;"
                    "    border-bottom-left-radius: %3px;"
                    "    border-bottom-right-radius: %3px;"
                    "}"
                    "QToolBar QWidget {"
                    "    margin: 0px;"
                    "}").arg(theme.backgroundColor.name()).arg(cur.colors.highlight.name()).arg(cur.metrics.borderRadius).arg(cur.metrics.spacing).arg(toolbarBg.name()).arg(toolbarText.name());

                editor->setStyleSheet(editorSs);
                editor->setFont(theme.editorFont);
            }
}
