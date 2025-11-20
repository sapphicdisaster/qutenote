#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QColor>
#include <QFont>
#include <QMap>
#include <QString>

#include "smartpointers.h"
#include <QMap>
#include <QStringList>
#include "texteditor.h"
#include <QTabWidget>

class TextEditor;
class FileBrowser;
class QWidget;
class QSplitter;
class QComboBox;
class QSpinBox;
class QComboBox;

// Base theme colors
struct ThemeColors {
    QColor primary;        // Main brand color
    QColor secondary;      // Secondary brand color
    QColor background;     // Main background
    QColor surface;        // Surface elements background
    QColor base;          // Base color for text entry widgets
    QColor text;          // Main text color
    QColor textSecondary; // Secondary text color
    QColor accent;        // Highlight/accent color
    QColor menuBackground; // Background for menus/toolbars
    QColor editorMenuBackground; // Optional darker background for editor toolbars
    QColor clicked;        // Color to use for clicked/checked buttons
    QColor border;        // Border color
    QColor error;         // Error/warning color
    QColor success;       // Success color
    QColor toolbarTextIcon; // Color for toolbar text/icons (allows icon inheritance)
    QColor highlight;
};

// Editor-specific theme
struct EditorTheme {
    QColor textColor;
    QColor backgroundColor;
    QColor selectionColor;
    QColor selectionBackground;
    QColor lineNumberColor;
    QColor lineNumberBackground;
    QColor currentLineColor;
    QFont editorFont;
    int fontSize;
};

// FileBrowser-specific theme
struct BrowserTheme {
    QColor itemBackground;
    QColor alternateBackground;
    QColor headerBackground;
    QColor selectedItemText;
    QColor selectedItemBackground;
    QColor gridLineColor;
    int itemSpacing;
    int iconSize;
};

struct ThemeMetrics {
    int spacing;          // Base spacing unit
    int borderRadius;     // Border radius for elements
    int iconSize;        // Standard icon size
    int touchTarget;      // Minimum touch target size
};

struct Theme {
    QString name;
    QString displayName;
    bool isDark;
    ThemeColors colors;
    ThemeMetrics metrics;
    QFont defaultFont;
    QFont headerFont;
    QString switchStyle;
};

class ThemeManager : public QObject, public QuteNote::Singleton<ThemeManager>
{
    Q_OBJECT
    friend class QuteNote::Singleton<ThemeManager>;

public:
    
    void applyTheme(const QString &themeName);
    void saveTheme(const Theme &theme);
    QStringList availableThemes() const;
    Theme currentTheme() const;
    Theme loadTheme(const QString &themeName) const;
    // Live theme editing
    void setThemeColor(const QString &role, const QColor &color);
    QColor themeColor(const QString &role) const;
    
    // Component-specific theme getters
    EditorTheme editorTheme() const;
    BrowserTheme browserTheme() const;
    
    // Apply theme to specific components
    void applyThemeToEditor(TextEditor *editor, const EditorTheme &theme);
    void applyThemeToFileBrowser(FileBrowser *fileBrowser);
    void applyThemeToSplitter(QSplitter *splitter);
    void applyThemeToComboBox(QComboBox *combo);
    void applyThemeToSpinBox(QSpinBox *spin);
    void applyThemeToComboBoxesInWidget(QWidget *parent);
    void applyThemeToSpinBoxesInWidget(QWidget *parent);
    void applyThemeToTabWidget(QTabWidget *tabWidget);
    void applyCurrentThemeStyles();

signals:
    void themeChanged(const Theme &newTheme);
    void editorThemeChanged(const EditorTheme &newTheme);
    // Emitted when a theme application starts/finishes. Useful for showing
    // UI feedback while the application computes and applies the theme.
    void themeApplyStarted();
    void themeApplyFinished();

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager() = default;
    
    void initializeDefaultThemes();
    void createPinkTheme();
    void createPurpleTheme();
    void applyThemeToApplication(const Theme &theme);
    QString themeFilePath(const QString &themeName) const;

    Theme m_currentTheme;
    QMap<QString, Theme> m_themes;
};

#endif // THEMEMANAGER_H
