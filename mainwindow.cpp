#include "mainwindow.h"
#include <QtCore/qglobal.h>
#include "./ui_mainwindow.h"
#include "filebrowser.h"
#include "titlebarwidget.h"
#include "thememanager.h"
#include "texteditor.h"

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
#include <QDir>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_stackedWidget(nullptr)
    , m_mainView(nullptr)
    , m_settingsView(nullptr)
    , m_touchInteraction(nullptr)
    , m_transitionAnimation(nullptr)
    , m_statusBar(nullptr)
    , m_backPressCount(0)
{
    // First set up the basic UI from the .ui file
    m_themeManager = QuteNote::Singleton<ThemeManager>::instance();
    m_titleBarWidget = new TitleBarWidget(this);
    m_titleBarWidget->setThemeManager(m_themeManager);
    if (ui) {
        ui->setupUi(this);
    }
    
    // Then set up our custom UI
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

    // (diagnostic startup dump removed)
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupTouchInteraction()
{
    // Enable touch interaction for the main window
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PinchGesture);
    
    // Set up any touch-specific behavior here
    // For now, we'll just ensure the window is touch-friendly
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
    // If the text editor has focus, let it handle all key presses except the back button.
    if (m_mainView && m_mainView->textEditor() && m_mainView->textEditor()->hasFocus()) {
        if (event->key() != Qt::Key_Back && event->key() != Qt::Key_Escape) {
            QMainWindow::keyReleaseEvent(event);
            return;
        }
    }

    // Handle Android back button (Qt::Key_Back)
    if (event->key() == Qt::Key_Back || event->key() == Qt::Key_Escape) {
        // Check if we're currently in the settings view
        if (m_stackedWidget && m_stackedWidget->currentWidget() == m_settingsView) {
            // Navigate back to main view
            showMainView();
            event->accept();
            return;
        }
        
        // Check if main view exists and has a file browser
        if (m_mainView) {
            // Get the file browser from main view
            FileBrowser* fileBrowser = m_mainView->fileBrowser();
            
            // If file browser exists, check if sidebar is visible
            if (fileBrowser) {
                // Get sidebar toggle button
                QToolButton* toggleBtn = m_mainView->sidebarToggleButton();
                
                // If sidebar is visible, hide it instead of exiting
                if (toggleBtn && toggleBtn->isChecked()) {
                    // Toggle sidebar to hide it
                    m_mainView->toggleSidebar(false);
                    toggleBtn->setChecked(false);
                    event->accept();
                    return;
                }
            }
        }
        
        // If we're already in the main view and sidebar is hidden, show exit confirmation
        m_backPressCount++;
        
        if (m_backPressCount >= 2) {
            // Check if there are unsaved changes
            if (m_mainView && m_mainView->textEditor() && m_mainView->textEditor()->isModified()) {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    tr("Unsaved Changes"),
                    tr("You have unsaved changes. Do you want to save before exiting?"),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
                );
                
                if (reply == QMessageBox::Save) {
                    // Save the document
                    m_mainView->saveFile();
                    // Continue to exit confirmation
                } else if (reply == QMessageBox::Cancel) {
                    // User cancelled, reset counter and don't exit
                    m_backPressCount = 0;
                    event->accept();
                    return;
                }
                // If Discard, continue to exit confirmation
            }
            
            // Show exit confirmation dialog
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, 
                tr("Exit QuteNote"), 
                tr("Are you sure you want to exit QuteNote?"),
                QMessageBox::Yes | QMessageBox::No
            );
            
            if (reply == QMessageBox::Yes) {
                // On Android, properly clean up touch interactions before quitting
#ifdef Q_OS_ANDROID
                // Process any pending touch events
                QCoreApplication::processEvents();
                
                // Give some time for touch interactions to settle
                // Use QApplication instance directly to avoid capturing 'this'
                QTimer::singleShot(100, qApp, &QApplication::quit);
#else
                QApplication::quit();
#endif
            } else {
                m_backPressCount = 0; // Reset counter if user cancels
            }
        } else {
            // Show a status bar message for the first back press
            if (m_statusBar) {
                m_statusBar->showMessage(tr("Press back again to exit"), 2000);
            }
            
            // Reset the counter after a short delay
            // Store a QPointer to safely check if MainWindow still exists
            QPointer<MainWindow> self(this);
            QTimer::singleShot(2000, this, [self]() {
                if (self) {
                    self->m_backPressCount = 0;
                }
            });
        }
        
        event->accept();
        return;
    }
    
    // For other keys, use default behavior
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

    // Create stacked widget for switching between views
    m_stackedWidget = new QStackedWidget(this);

    // Set up touch interaction first
    setupTouchInteraction();

    // Create main view and settings view
    m_mainView = new MainView(this);
    m_settingsView = new SettingsView(this);

    // Initialize the settings view component
    if (m_settingsView) {
        m_settingsView->initializeComponent();
        // setupComponent() is called automatically during initialization
    }
    // MainView inherits from QWidget, not ComponentBase, so no initializeComponent needed
    
    // Set the default save directory for the text editor
    if (m_mainView && m_mainView->textEditor()) {
        m_mainView->textEditor()->setDefaultSaveDirectory(m_mainView->rootDirectory());
    }

    // Check for null pointers before adding to stacked widget
    if (m_mainView && m_settingsView && m_stackedWidget) {
        // Add views to stacked widget
        m_stackedWidget->addWidget(m_mainView);
        m_stackedWidget->addWidget(m_settingsView);

        // Set central widget
        setCentralWidget(m_stackedWidget);
        // (No connection to MainView::currentFileChanged, as it does not exist)
    }

    // Connect signals only if views were created successfully
    if (m_mainView && m_settingsView) {
        connect(m_mainView, &MainView::settingsRequested,
                this, &MainWindow::showSettings);
        connect(m_settingsView, &SettingsView::settingsChanged,
                this, &MainWindow::applySettings);
        connect(m_settingsView, &SettingsView::backToMain,
                this, &MainWindow::showMainView);
        connect(m_mainView, &MainView::fileSaved, this, [this](const QString &filePath) {
            Q_UNUSED(filePath);
            if (m_mainView && m_mainView->fileBrowser()) {
                m_mainView->fileBrowser()->populateTree();
            }
        });
    }

    // Set window properties
#ifndef Q_OS_ANDROID
    setWindowTitle("QuteNote");
#endif
    setMinimumSize(800, 600);
    resize(1200, 800);

    // Create status bar
    m_statusBar = statusBar();
    m_statusBar->showMessage(tr("Ready"));
    
    // Set initial height to half of default
    m_statusBar->setFixedHeight(20);
    
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

void MainWindow::applySettings()
{
    if (!m_settingsView || !m_mainView) return;
    
    // Get the settings object from the settings view
    QSettings* settings = new QSettings("QuteNote", "QuteNote");
    
    // Apply notes directory setting
    QString notesDir = settings->value("notesDirectory", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QuteNote").toString();
    m_mainView->setRootDirectory(notesDir);
    
    // Apply sidebar visibility setting
    bool showSidebar = settings->value("showSidebarByDefault", true).toBool();
    m_mainView->toggleSidebar(showSidebar);
    
    delete settings;
    
    showMainView();
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

void MainWindow::showMainView()
{
    if (!m_stackedWidget || !m_mainView) return;
    m_stackedWidget->setCurrentWidget(m_mainView);
    #ifndef Q_OS_ANDROID
    setWindowTitle("QuteNote");
    #endif
    m_backPressCount = 0; // Reset back press counter when returning to main view

    // (temporary diagnostics removed)
}

void MainWindow::showSettings()
{
    if (!m_stackedWidget || !m_settingsView) return;
    m_stackedWidget->setCurrentWidget(m_settingsView);
#ifndef Q_OS_ANDROID
    setWindowTitle("QuteNote - Settings");
#endif
    m_backPressCount = 0; // Reset back press counter when entering settings
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
