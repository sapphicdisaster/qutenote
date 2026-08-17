#include "mainwindow.h"
#include "filebrowser.h"
#include "titlebarwidget.h"
#include "thememanager.h"
#include "texteditor.h"
#include "uiutils.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QApplication>
#include <QLabel>
#include <QTimer>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QPointer>

#ifdef Q_OS_ANDROID
#include <QWindow>
#include <QGuiApplication>
#include <QScreen>
#endif

#include <QPainter>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mainView(nullptr)
    , m_touchInteraction(nullptr)
    , m_transitionAnimation(nullptr)
    , m_statusBar(nullptr)
    , m_backPressCount(0)
{
    m_themeManager = QuteNote::Singleton<ThemeManager>::instance();
    m_titleBarWidget = new TitleBarWidget(this);
    m_titleBarWidget->setThemeManager(m_themeManager);

    setupUI();

    // Insert the title widget into the MainView's toolbar so it's centered in the
    // application's main toolbar (appearing before the Undo button).
    if (m_mainView) {
        m_mainView->setTitleWidget(m_titleBarWidget);
    } else {
        // If main view wasn't created yet or is null for some reason, keep it as
        // the menu widget fallback so it's still visible.
        setMenuWidget(m_titleBarWidget);
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupTouchInteraction()
{
    // Enable touch interaction for the main window
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PinchGesture);
    
    // Set up any touch-specific behavior here
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Handle resize events on debug-overlay parents so overlays track size.
    if (event && event->type() == QEvent::Resize) {
        QWidget *w = qobject_cast<QWidget*>(watched);
        if (w) {
            QWidget *dbg = w->findChild<QWidget*>("DBG_StackOverlay");
            if (dbg && dbg->parentWidget() == w) {
                dbg->setGeometry(w->rect());
            }
            QWidget *dbg2 = w->findChild<QWidget*>("DBG_MainViewOverlay");
            if (dbg2 && dbg2->parentWidget() == w) {
                dbg2->setGeometry(w->rect());
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    // If the text editor has focus, let it handle keys except back/escape
    if (m_mainView && m_mainView->textEditor() && m_mainView->textEditor()->hasFocus()) {
        if (event->key() != Qt::Key_Back && event->key() != Qt::Key_Escape) {
            QMainWindow::keyReleaseEvent(event);
            return;
        }
    }

    // Handle Android back button or Escape
    if (event->key() == Qt::Key_Back || event->key() == Qt::Key_Escape) {
        // If the right settings panel is open, close it
        if (m_mainView && m_mainView->isPanelOpen(MainView::PanelSide::Right)) {
            m_mainView->togglePanel(MainView::PanelSide::Right, false);
            event->accept();
            return;
        }

        // Double-press to exit: first press shows a prompt
        if (m_backPressCount == 0) {
            if (m_statusBar) {
                m_statusBar->showMessage(tr("Press back again to exit"), 2000);
            }
            m_backPressCount = 1;
            QPointer<MainWindow> self(this);
            QTimer::singleShot(2000, this, [self]() {
                if (self) self->m_backPressCount = 0;
            });
            event->accept();
            return;
        }

        // Second press: quit the application
        UIUtils::quitApplication();
        event->accept();
        return;
    }

    // Default behavior for other keys
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    // Call base paint to keep normal rendering
    QMainWindow::paintEvent(event);

    // No additional painting here (debug overlay removed).
}

void MainWindow::setupUI()
{
    if (m_titleBarWidget) {
        connect(m_titleBarWidget, &TitleBarWidget::filenameChanged, this, &MainWindow::onTitleBarFilenameChanged);
        connect(m_titleBarWidget, &TitleBarWidget::saveRequested, this, [this]() {
            if (m_mainView) {
                m_mainView->saveFile();
            }
        });
    }

    // Set up touch interaction first
    setupTouchInteraction();

    // Create main view — owns the editor, file browser, and settings panel internally
    m_mainView = new MainView(this);
    
    // Set the default save directory for the text editor
    if (m_mainView && m_mainView->textEditor()) {
        m_mainView->textEditor()->setDefaultSaveDirectory(m_mainView->rootDirectory());
    }

    // MainView is the sole central widget
    setCentralWidget(m_mainView);

    // Connect signal for when a file is saved (refresh file browser)
    connect(m_mainView, &MainView::fileSaved, this, [this](const QString &filePath) {
        Q_UNUSED(filePath);
        if (m_mainView && m_mainView->fileBrowser()) {
            m_mainView->fileBrowser()->populateTree();
        }
    });

    // Set window properties
    UIUtils::setupDesktopWindow(this, "QuteNote", 800, 600, 1200, 800);

    // Create status bar
    m_statusBar = statusBar();
    m_statusBar->showMessage(tr("Ready"));
    
    // Set initial height to half of default
    m_statusBar->setFixedHeight(20);
    
    // Hide the built-in separator QFrame between central widget and status bar.
    // QMainWindow::separator CSS doesn't override native style on all platforms.
    const auto qframes = findChildren<QFrame*>();
    for (QFrame* f : qframes) {
        if (f->objectName().isEmpty() && f->parentWidget() == this) {
            f->setFixedHeight(0);
            f->hide();
        }
    }
    
    // Connect to theme changes to apply different colors to UI elements
    connect(m_themeManager, &ThemeManager::themeChanged, this, &MainWindow::onThemeChanged);
    // Apply initial theme
    onThemeChanged(m_themeManager->currentTheme());

#ifdef Q_OS_ANDROID
    // Apply Android system UI styling after a short delay to ensure window is created
    QTimer::singleShot(100, this, &MainWindow::setupAndroidSystemUI);
#endif
}

void MainWindow::onTitleBarFilenameChanged(const QString &newName)
{
    if (!m_mainView) return;
    QString oldPath = m_mainView->currentFile();
    if (oldPath.isEmpty()) return;
    QFileInfo fi(oldPath);
    QString dir = fi.absolutePath();
    // If newName already contains an extension, use it as-is; otherwise preserve the old extension
    QFileInfo newInfo(newName);
    QString newPath;
    if (!newInfo.suffix().isEmpty()) {
        newPath = dir + "/" + newName;
    } else {
        const QString ext = fi.suffix();
        newPath = dir + "/" + newName + (ext.isEmpty() ? QString() : ("." + ext));
    }

    if (QFile::exists(newPath)) {
        QMessageBox::warning(this, tr("Rename Failed"), tr("A file with that name already exists."));
        return;
    }

    if (QFile::rename(oldPath, newPath)) {
        m_mainView->onFileSelected(newPath);
        if (m_mainView->fileBrowser())
            m_mainView->fileBrowser()->populateTree();
    } else {
        QMessageBox::warning(this, tr("Rename Failed"), tr("Could not rename file."));
    }

}

void MainWindow::onThemeChanged(const Theme &newTheme)
{
    // Delegate theme application to the ThemeManager to ensure all styles are
    // applied globally from a single source of truth.
    if (m_themeManager) {
        m_themeManager->applyCurrentThemeStyles();
    }

    // If we have a main view, ensure its text editor gets the proper theme
    if (m_mainView && m_mainView->textEditor()) {
        ThemeManager::instance()->applyThemeToEditor(m_mainView->textEditor(), ThemeManager::instance()->editorTheme());
    }

#ifdef Q_OS_ANDROID
    // Update Android system UI colors to match the new theme
    setupAndroidSystemUI();
#endif
}

#ifdef Q_OS_ANDROID
void MainWindow::setupAndroidSystemUI()
{
    // Get the current theme's menu/plate color for the system bars
    if (!m_themeManager) return;
    
    Theme theme = m_themeManager->currentTheme();
    QColor plateColor = theme.colors.menuBackground.isValid()
        ? theme.colors.menuBackground.darker(110)
        : theme.colors.background.darker(130);
    
    // Get the main window
    QWindow *window = this->windowHandle();
    if (!window) return;
    
    // Convert QColor to Android ARGB format (0xAARRGGBB)
    // Android expects fully opaque colors for system bars
    int alpha = 255; // Fully opaque
    int red = plateColor.red();
    int green = plateColor.green();
    int blue = plateColor.blue();
    int androidColor = (alpha << 24) | (red << 16) | (green << 8) | blue;
    
    // Determine if we should use light or dark icons based on background brightness
    bool useLightIcons = plateColor.lightness() < 128;
    
    // Set status bar color and appearance
    window->setProperty("_q_android_statusBarColor", androidColor);
    window->setProperty("_q_android_lightStatusBar", !useLightIcons);
    
    // Set navigation bar color and appearance
    window->setProperty("_q_android_navigationBarColor", androidColor);
    window->setProperty("_q_android_lightNavigationBar", !useLightIcons);
    
    // Optional: Make status/navigation bars semi-transparent for modern Android look
    // Uncomment if you want translucent bars:
    // window->setProperty("_q_android_statusBarTranslucent", true);
    // window->setProperty("_q_android_navigationBarTranslucent", true);
}
#endif
