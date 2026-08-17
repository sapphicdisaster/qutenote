#ifndef UIUTILS_H
#define UIUTILS_H

#include <QWidget>
#include <QString>
#include <QMargins>
#include <QSize>
#include <QToolButton>
#include <QToolBar>
#include <QActionGroup>
#include <QScrollBar>
#include <QScrollArea>
#include <QStandardPaths>
#include <QSettings>

class QToolBar;
class QPushButton;
class QLabel;
class QMainWindow;

namespace UIUtils {

struct TouchMetrics {
    static const int MINIMUM_TOUCH_TARGET = 48;      // Minimum size for touch targets (Android recommended)
    static const int PREFERRED_TOUCH_TARGET = 56;    // Preferred size for touch targets
    static const int LARGE_TOUCH_TARGET = 64;        // Large size for primary touch targets
    static const int SPACING = 8;                    // Default spacing between elements
    static const int MARGIN = 12;                    // Default margin for containers
};

// Shared component creation
QWidget* createCollapsibleSection(const QString& title, QWidget* content, 
                                bool isExpanded = true, QWidget* parent = nullptr);
QWidget* createToolbarSection(const QString& title, const QList<QWidget*>& items,
                            bool isVertical = false, QWidget* parent = nullptr);
QToolBar* createTouchFriendlyToolbar(bool isVertical = false, QWidget* parent = nullptr);

// Widget setup helpers
void setupTouchFeedback(QWidget* widget);
void makeTouchFriendly(QWidget* widget, bool scaleContents = false);
void setupScrollArea(QWidget* area, bool enableTouch = true);

// Button creation helpers
QPushButton* createActionButton(const QString& text, const QString& iconName = QString(),
                              QWidget* parent = nullptr);
QToolButton* createToolButton(const QString& text, const QString& iconName = QString(),
                            QWidget* parent = nullptr);

// Layout helpers
QMargins getTouchFriendlyMargins();
int getTouchFriendlySpacing();
QSize getTouchFriendlySize(bool isLarge = false);

// Status indicators
QLabel* createStatusLabel(const QString& text, QWidget* parent = nullptr);
void updateStatusLabel(QLabel* label, const QString& text, bool isError = false);

// Status bar
void showStatusMessage(QWidget* source, const QString& message, int timeoutMs = 3000);

// Default notes directory
QString defaultNotesDirectory();

// Shared QSettings accessor — provides a single consistent QSettings instance
// using organization "QuteNote" and application "QuteNote".
QSettings& quteSettings();

// Platform-specific adaptations
bool isMobileDevice();
void adaptForMobile(QWidget* widget);
void adaptForTouch(QWidget* widget);

// Apply desktop-only window setup (title, minimum size, default size).
// No-op on Android where the OS manages window dimensions.
void setupDesktopWindow(QWidget* window, const QString& title,
                        int minWidth, int minHeight,
                        int defaultWidth, int defaultHeight);

// Quit the application safely across platforms.
// On Android uses QTimer::singleShot to avoid event-loop issues,
// on desktop calls quit() directly.
void quitApplication();

// Show a window correctly on each platform.
// Calls showMaximized() on Android, show() on desktop.
void showWindow(QWidget* window);

} // namespace UIUtils

#endif // UIUTILS_H