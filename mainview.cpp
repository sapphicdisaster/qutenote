#include "mainview.h"
#include <QtCore/qglobal.h>
#include "texteditor.h"
#include "filebrowser.h"
#include "thememanager.h"
#include "titlebarwidget.h"

#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QProgressBar>
#include <QPushButton>
#include <QToolBar>
#include <QAction>
#include <QMenuBar>
#include <QStatusBar>
#include <QPropertyAnimation>
#include <QTimer>
#include <QApplication>
#include <QEventLoop>
#include <QScreen>
#include <QFileInfo>
#include <QTabWidget>
#include <QFileSystemWatcher>
#include <QTextEdit>
#include <QMainWindow>
#include <QCloseEvent>
#include <QScrollArea>
#include <QScroller>
#include <QScrollBar>
#include <QFontMetrics>

MainView::MainView(QWidget *parent)
    : QWidget(parent)
    , m_rootDirectory(QSettings().value("notesDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/QuteNote").toString())
    , m_currentFile(QString())
    , m_sidebarVisible(QSettings().value("showSidebarByDefault", true).toBool())
    , m_sidebarWidth(250) // Default sidebar width
    , m_titleBarWidget(nullptr)
    , m_sidebar(nullptr)
    , m_fileBrowser(nullptr)
    , m_textEditor(nullptr)
    , m_toggleSidebarBtn(nullptr)
    , m_sidebarLayout(nullptr)
    , m_mainLayout(nullptr)
    , m_resizeTimer(nullptr)
{
    // Set window properties for better mobile experience

#ifndef Q_OS_ANDROID
    setWindowTitle("QuteNote");
    setMinimumSize(360, 600);
#else
    // On Android, set the height to available screen geometry to avoid being cut off by system UI
    QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    setGeometry(avail);
    setMinimumSize(avail.width(), avail.height());
    setMaximumSize(avail.width(), avail.height());
#endif

    // Enable touch events and gestures
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::SwipeGesture);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);

    // Create main layout first
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Setup UI components
    setupUI();
    setupMenus();
    setupToolbar();
    
    // Apply initial sidebar visibility from settings
    if (m_toggleSidebarBtn) {
        m_toggleSidebarBtn->setChecked(m_sidebarVisible);
    }
    if (m_sidebar) {
        m_sidebar->setVisible(m_sidebarVisible);
    }

    // Connect signals
    if (m_fileBrowser) {
        connect(m_fileBrowser, &FileBrowser::fileSelected,
                this, &MainView::onFileSelected);
    }
    
    if (m_textEditor) {
        connect(m_textEditor, &TextEditor::fileSaved,
                this, &MainView::onFileSaved);
        connect(m_textEditor, &TextEditor::modificationChanged,
                this, &MainView::onEditorModified);
    }

    // Set initial directory
    setRootDirectory(m_rootDirectory);
    
    // Show the window after everything is set up
    show();
}

MainView::~MainView()
{
}

void MainView::onThemeChanged(const Theme &newTheme)
{
    // Apply theme to text editor if it exists
    if (m_textEditor) {
        ThemeManager::instance()->applyThemeToEditor(m_textEditor, ThemeManager::instance()->editorTheme());
    }
    
    // Apply theme to file browser if it exists
    if (m_fileBrowser) {
        ThemeManager::instance()->applyThemeToFileBrowser(m_fileBrowser);
    }
}

void MainView::onThemeApplyStarted()
{
    if (!m_themeOverlay || !m_themeProgressBar) return;
    // Ensure overlay covers the whole MainView area
    m_themeOverlay->setGeometry(this->rect());
    m_themeOverlay->raise();
    m_themeOverlay->show();
    m_themeProgressBar->setRange(0, 0);
    qApp->processEvents();
}

void MainView::onThemeApplyFinished()
{
    if (!m_themeOverlay) return;
    m_themeOverlay->hide();
}

// Apply overlay style to overscroll indicators (extracted from setupToolbar)
void MainView::applyOverlayStyleToMain()
{
    if (!m_overscrollLeftWidget || !m_overscrollRightWidget) return;
    Theme theme = ThemeManager::instance()->currentTheme();
    const int height = qMax(36, theme.metrics.touchTarget - 8);
    const int width = qMax(18, height / 2); // make overscroll half as wide as tall
    const int iconSz = qMax(20, theme.metrics.iconSize);

    auto setBtn = [&](QToolButton* btn){
        if (!btn) return;
        // Force an exact fixed size so external style rules or layouts cannot expand the button
        // (ensures width == ~height/2 as intended).
        btn->setFixedSize(width, height);
        btn->setIconSize(QSize(iconSz, iconSz));
        btn->setCursor(Qt::PointingHandCursor);

        const QColor base = theme.colors.text;
        const int r = base.red();
        const int g = base.green();
        const int b = base.blue();
        const int radius = theme.metrics.borderRadius;
        const QString ss = QString(
            "QToolButton {"
            "  border: none;"
            "  background: rgba(%1,%2,%3,0.08);"
            "  padding: 2px;"
            "  margin: 2px;"
            "  border-radius: %4px;"
            "}"
            "QToolButton:hover {"
            "  background: rgba(%1,%2,%3,0.14);"
            "}"
            "QToolButton:pressed {"
            "  background: rgba(%1,%2,%3,0.20);"
            "}"
        ).arg(r).arg(g).arg(b).arg(radius);
        btn->setStyleSheet(ss);
    };

    setBtn(m_overscrollLeftWidget);
    setBtn(m_overscrollRightWidget);

    // Apply touch attributes
    m_overscrollLeftWidget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    m_overscrollLeftWidget->setFocusPolicy(Qt::NoFocus);
    m_overscrollLeftWidget->setCursor(Qt::PointingHandCursor);
    m_overscrollLeftWidget->setAttribute(Qt::WA_StaticContents, false);

    m_overscrollRightWidget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    m_overscrollRightWidget->setFocusPolicy(Qt::NoFocus);
    m_overscrollRightWidget->setCursor(Qt::PointingHandCursor);
    m_overscrollRightWidget->setAttribute(Qt::WA_StaticContents, false);
}

void MainView::applyToggleStyle()
{
    if (!m_toggleSidebarBtn) return;
    Theme theme = ThemeManager::instance()->currentTheme();
    // Use a slightly darker variant of the theme's menuBackground so the toggle
    // plate reads as on-par or darker than toolbar buttons (use ThemeManager values)
    QColor plateBg = theme.colors.menuBackground.isValid() ? theme.colors.menuBackground.darker(110)
                                                            : theme.colors.background.darker(130);
    QString ss = QString("QToolButton { background: %1; color: %2; border: none; border-radius: 6px; }")
        .arg(plateBg.name()).arg(theme.colors.toolbarTextIcon.name());
    // Do not set a widget-local stylesheet here; ThemeManager's application
    // stylesheet will style toolbar and fixed-area buttons. Keep icon size.
    m_toggleSidebarBtn->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
}

void MainView::applySettingsStyle()
{
    if (!m_settingsBtn) return;
    Theme theme = ThemeManager::instance()->currentTheme();
    m_settingsBtn->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
    // Use a plate background variant so the settings button sits on the same/darker plate
    QColor plateBg = theme.colors.menuBackground.isValid() ? theme.colors.menuBackground.darker(110)
                                                            : theme.colors.background.darker(130);
    // Do not set a widget-local stylesheet here; ThemeManager will apply
    // the correct plate and hover states for buttons in the toolbar row.
    m_settingsBtn->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
}

void MainView::resizeEvent(QResizeEvent *event)
{
    // Let the layout system handle the resizing
    QWidget::resizeEvent(event);
    
    // Simple update for proper resizing - Qt's layout system should handle most of this
    if (m_textEditor) {
        m_textEditor->updateGeometry();
    }
    
    if (m_splitter) {
        m_splitter->updateGeometry();
    }
    
    // Force layout update
    if (layout()) {
        layout()->activate();
    }
}

void MainView::closeEvent(QCloseEvent *event)
{
    // Check for unsaved changes before closing
    if (promptSaveIfModified()) {
        event->accept();  // Allow the window to close
    } else {
        event->ignore();  // User cancelled, don't close
    }
}

void MainView::setupUI()
{
    // Create sidebar with flexible sizing

    m_sidebar = new QWidget(this);
    m_sidebar->setMinimumWidth(150);  // Reasonable minimum width
    m_sidebar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create file browser

    m_fileBrowser = new FileBrowser(m_sidebar);
    m_fileBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Setup sidebar layout
    m_sidebarLayout = new QVBoxLayout(m_sidebar);
    m_sidebarLayout->setContentsMargins(0, 0, 0, 0);
    m_sidebarLayout->setSpacing(0);
    m_sidebarLayout->addWidget(m_fileBrowser);

    // Create text editor with proper flexbox-like behavior

    m_textEditor = new TextEditor(this);
    m_textEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Create splitter for flexible layout

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(8);  // Larger handle for easier resizing
    m_splitter->setChildrenCollapsible(true);  // Allow widgets to collapse (needed for hide/show)
    m_splitter->setOpaqueResize(true);  // Enable opaque resizing for smoother experience
    m_splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Style the splitter handle
    m_splitter->setStyleSheet(
        "QSplitter::handle { "
        "  background: palette(mid); "
        "  border: 1px solid palette(dark); "
        "}"
        "QSplitter::handle:hover { "
        "  background: palette(highlight); "
        "}"
    );
    
    // Add widgets to splitter

    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_textEditor);
    
    // Set stretch factors for flexbox-like behavior

    m_splitter->setStretchFactor(0, 0);  // Sidebar maintains its size
    m_splitter->setStretchFactor(1, 1);  // Editor takes all remaining space
    
    // Set initial splitter sizes (sidebar: 250px, editor: remaining)

    // Only set initial sizes if not on Android (let layout fill on Android)
#ifndef Q_OS_ANDROID
    m_splitter->setSizes({250, 1000});
#endif
    
    // Add splitter to the main layout

    m_mainLayout->addWidget(m_splitter);

    // On Android, ensure the splitter fills the available space
#ifdef Q_OS_ANDROID
    m_mainLayout->setStretch(0, 1);
#endif
    
    // Connect signals
    connect(m_fileBrowser, &FileBrowser::fileSelected,
            this, &MainView::onFileSelected);
    connect(m_textEditor, &TextEditor::modificationChanged,
            this, &MainView::onEditorModified);
    
    // Connect to theme changes
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainView::onThemeChanged);
    connect(ThemeManager::instance(), &ThemeManager::themeApplyStarted, this, &MainView::onThemeApplyStarted);
    connect(ThemeManager::instance(), &ThemeManager::themeApplyFinished, this, &MainView::onThemeApplyFinished);
    // Apply initial theme
    onThemeChanged(ThemeManager::instance()->currentTheme());

    // Create a lightweight overlay to display while a theme is being applied.
    // This overlay is intentionally simple and uses an indeterminate progress
    // bar so users get immediate feedback when themes change.
    m_themeOverlay = new QWidget(this);
    m_themeOverlay->setObjectName("ThemeOverlay");
    m_themeOverlay->setVisible(false);
    m_themeOverlay->setAttribute(Qt::WA_NoSystemBackground, false);
    m_themeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_themeOverlay->setStyleSheet("QWidget#ThemeOverlay { background: rgba(0,0,0,0.22); }");
    QVBoxLayout *overlayLayout = new QVBoxLayout(m_themeOverlay);
    overlayLayout->setContentsMargins(0,0,0,0);
    overlayLayout->addStretch();
    m_themeProgressBar = new QProgressBar(m_themeOverlay);
    m_themeProgressBar->setFixedWidth(240);
    m_themeProgressBar->setRange(0, 0); // indeterminate
    m_themeProgressBar->setTextVisible(false);
    overlayLayout->addWidget(m_themeProgressBar, 0, Qt::AlignHCenter);
    overlayLayout->addStretch();
}

void MainView::setupMenus()
{
    m_menuBar = new QMenuBar();
    m_menuBar->setStyleSheet(
        "QMenuBar { "
        "  background: palette(window); "
        "  border-bottom: 1px solid palette(mid); "
        "} "
        "QMenuBar::item { "
        "  padding: 8px 12px; "
        "  background: transparent; "
        "} "
        "QMenuBar::item:selected { "
        "  background: palette(highlight); "
        "  color: palette(highlighted-text); "
        "} "
        "QMenuBar::item:pressed { "
        "  background: palette(dark); "
        "  color: palette(light); "
        "} "
        "QMenu { "
        "  border: 1px solid palette(mid); "
        "  border-radius: 4px; "
        "} "
        "QMenu::item { "
        "  padding: 8px 24px; "
        "  border-radius: 2px; "
        "} "
        "QMenu::item:selected { "
        "  background: palette(highlight); "
        "  color: palette(highlighted-text); "
        "}"
    );

    // File menu
    QMenu *fileMenu = m_menuBar->addMenu("&File");

    m_newAction = fileMenu->addAction("&New");
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, [this]() {
        updateStatusBar("Creating new document...", 1000);
        newFile();
    });

    m_openAction = fileMenu->addAction("&Open");
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, [this]() {
        updateStatusBar("Opening file dialog...", 1000);
        QString fileName = QFileDialog::getOpenFileName(this,
            "Open Document", m_fileBrowser->currentDirectory(),
            "HTML files (*.html);;Text files (*.txt);;All files (*.*)");
        if (!fileName.isEmpty()) {
            QFileInfo fi(fileName);
            updateStatusBar("Opening file: " + fi.fileName(), 2000);
            loadFile(fileName);
        }
    });

    fileMenu->addSeparator();

    m_saveAction = fileMenu->addAction("&Save");
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainView::saveFile);

    fileMenu->addSeparator();

    m_settingsAction = fileMenu->addAction("&Settings");
    connect(m_settingsAction, &QAction::triggered, this, &MainView::showSettings);

    fileMenu->addSeparator();

    m_exitAction = fileMenu->addAction("E&xit");
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    // Edit menu
    QMenu *editMenu = m_menuBar->addMenu("&Edit");

    // Only set up edit menu actions if text editor exists
    if (m_textEditor) {
        m_undoAction = editMenu->addAction("&Undo");
        m_undoAction->setShortcut(QKeySequence::Undo);
        connect(m_undoAction, &QAction::triggered, m_textEditor, &TextEditor::undo);

        m_redoAction = editMenu->addAction("&Redo");
        m_redoAction->setShortcut(QKeySequence::Redo);
        connect(m_redoAction, &QAction::triggered, m_textEditor, &TextEditor::redo);

        editMenu->addSeparator();

        m_cutAction = editMenu->addAction("Cu&t");
        m_cutAction->setShortcut(QKeySequence::Cut);
        connect(m_cutAction, &QAction::triggered, m_textEditor, &TextEditor::cut);

        m_copyAction = editMenu->addAction("&Copy");
        m_copyAction->setShortcut(QKeySequence::Copy);
        connect(m_copyAction, &QAction::triggered, m_textEditor, &TextEditor::copy);

        m_pasteAction = editMenu->addAction("&Paste");
        m_pasteAction->setShortcut(QKeySequence::Paste);
        connect(m_pasteAction, &QAction::triggered, m_textEditor, &TextEditor::paste);
    } else {
        // Create disabled actions if no text editor
        m_undoAction = editMenu->addAction("&Undo");
        m_undoAction->setShortcut(QKeySequence::Undo);
        m_undoAction->setEnabled(false);
        
        m_redoAction = editMenu->addAction("&Redo");
        m_redoAction->setShortcut(QKeySequence::Redo);
        m_redoAction->setEnabled(false);
        
        editMenu->addSeparator();
        
        m_cutAction = editMenu->addAction("Cu&t");
        m_cutAction->setShortcut(QKeySequence::Cut);
        m_cutAction->setEnabled(false);
        
        m_copyAction = editMenu->addAction("&Copy");
        m_copyAction->setShortcut(QKeySequence::Copy);
        m_copyAction->setEnabled(false);
        
        m_pasteAction = editMenu->addAction("&Paste");
        m_pasteAction->setShortcut(QKeySequence::Paste);
        m_pasteAction->setEnabled(false);
    }

    // View menu
    QMenu *viewMenu = m_menuBar->addMenu("&View");
    QAction *toggleSidebarAction = viewMenu->addAction("Toggle &Sidebar");
    connect(toggleSidebarAction, &QAction::triggered, this, &MainView::toggleSidebar);
    
    // Add menu bar to layout
    m_mainLayout->setMenuBar(m_menuBar);
}

void MainView::setupToolbar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setMovable(false);
    Theme theme = ThemeManager::instance()->currentTheme();
    m_toolbar->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
    // Make toolbar content-width driven so the scroll area can show overflow
    m_toolbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    // Initial toolbar setup — visual styling is applied centrally via ThemeManager
    // in applyToolbarStyle(). Keep toolbar configuration here but avoid applying a
    // redundant stylesheet so the theme-managed stylesheet is the single source of truth.
    m_toolbar->setContentsMargins(0, 0, 0, 0);
    // Ensure the toolbar widget honors stylesheet background painting
    m_toolbar->setAttribute(Qt::WA_StyledBackground, true);
    m_toolbar->setAutoFillBackground(true);
    
    // Add toggle button with icon (chevrons-left when sidebar is visible, to indicate hiding it)
    QIcon sidebarIcon(":/resources/icons/custom/chevrons-left.svg");
    if (sidebarIcon.isNull()) {
        sidebarIcon = style()->standardIcon(QStyle::SP_TitleBarNormalButton);
    }
    m_toggleSidebarBtn = new QToolButton(this);
    m_toggleSidebarBtn->setCheckable(true);
    m_toggleSidebarBtn->setChecked(true);
    m_toggleSidebarBtn->setIcon(sidebarIcon);
    m_toggleSidebarBtn->setToolTip("Toggle Sidebar");
    m_toggleSidebarBtn->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
    // Make toggle narrower: icon width + small padding, but full touchTarget height
    int toggleWidth = theme.metrics.iconSize + 12; // icon + 6px padding each side
    m_toggleSidebarBtn->setFixedSize(toggleWidth, theme.metrics.touchTarget);
    // Do not add toggle to the scrollable toolbar; it will be placed in the left fixed area
    // Style the toggle to match the titlebar primary color
    // Apply initial and theme-updated toggle styling via member function
    applyToggleStyle();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainView::applyToggleStyle);
    
    // Add separator with reduced width
    QWidget *separator1 = new QWidget();
    separator1->setFixedWidth(6);
    m_toolbar->addWidget(separator1);
    
    // Set custom cute icons for actions with fallback to standard icons
    QIcon newIcon(":/resources/icons/custom/file-plus-2.svg");
    if (newIcon.isNull()) {
        newIcon = style()->standardIcon(QStyle::SP_FileIcon);
    }
    m_newAction->setIcon(newIcon);
    
    QIcon openIcon(":/resources/icons/custom/folder.svg");
    if (openIcon.isNull()) {
        openIcon = style()->standardIcon(QStyle::SP_DialogOpenButton);
    }
    m_openAction->setIcon(openIcon);
    
    QIcon saveIcon(":/resources/icons/custom/save.svg");
    if (saveIcon.isNull()) {
        saveIcon = style()->standardIcon(QStyle::SP_DialogSaveButton);
    }
    m_saveAction->setIcon(saveIcon);
    
    QIcon undoIcon(":/resources/icons/custom/undo.svg");
    if (undoIcon.isNull()) {
        undoIcon = style()->standardIcon(QStyle::SP_ArrowBack);
    }
    m_undoAction->setIcon(undoIcon);
    connect(m_undoAction, &QAction::triggered, this, [this]() {
        updateStatusBar("Undoing last action...", 1000);
        if (m_textEditor) m_textEditor->undo();
    });
    
    QIcon redoIcon(":/resources/icons/custom/redo.svg");
    if (redoIcon.isNull()) {
        redoIcon = style()->standardIcon(QStyle::SP_ArrowForward);
    }
    m_redoAction->setIcon(redoIcon);
    connect(m_redoAction, &QAction::triggered, this, [this]() {
        updateStatusBar("Redoing last action...", 1000);
        if (m_textEditor) m_textEditor->redo();
    });
    
    QIcon cutIcon(":/resources/icons/custom/cut.svg");
    if (cutIcon.isNull()) {
        cutIcon = style()->standardIcon(QStyle::SP_DialogCancelButton);
    }
    m_cutAction->setIcon(cutIcon);
    connect(m_cutAction, &QAction::triggered, this, [this]() {
        updateStatusBar("Cutting selected text...", 1000);
        if (m_textEditor) m_textEditor->cut();
    });
    
    QIcon copyIcon(":/resources/icons/custom/copy.svg");
    if (copyIcon.isNull()) {
        copyIcon = style()->standardIcon(QStyle::SP_CommandLink);
    }
    m_copyAction->setIcon(copyIcon);
    connect(m_copyAction, &QAction::triggered, this, [this]() {
        updateStatusBar("Copying selected text...", 1000);
        if (m_textEditor) m_textEditor->copy();
    });
    
    QIcon pasteIcon(":/resources/icons/custom/paste.svg");
    if (pasteIcon.isNull()) {
        pasteIcon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    }
    m_pasteAction->setIcon(pasteIcon);
    connect(m_pasteAction, &QAction::triggered, this, [this]() {
        updateStatusBar("Pasting clipboard content...", 1000);
        if (m_textEditor) m_textEditor->paste();
    });
    
    // Add main actions with reduced spacing
    m_toolbar->addAction(m_newAction);
    m_toolbar->addAction(m_openAction);
    m_toolbar->addAction(m_saveAction);
    
    // Add separator with reduced width
    QWidget *separator2 = new QWidget();
    separator2->setFixedWidth(6);
    m_toolbar->addWidget(separator2);
    
    m_toolbar->addAction(m_undoAction);
    m_toolbar->addAction(m_redoAction);
    
    // Add separator with reduced width
    QWidget *separator3 = new QWidget();
    separator3->setFixedWidth(6);
    m_toolbar->addWidget(separator3);
    
    m_toolbar->addAction(m_cutAction);
    m_toolbar->addAction(m_copyAction);
    m_toolbar->addAction(m_pasteAction);

    // Finalize toolbar sizing so the scroll area will know when content overflows
    recomputeToolbarContentWidth();
    // Recompute when the main window or style changes
    connect(qApp, &QApplication::paletteChanged, this, &MainView::recomputeToolbarContentWidth);
    connect(qApp, &QApplication::fontChanged, this, &MainView::recomputeToolbarContentWidth);

    // Apply toolbar-specific theme styles (height, checked color)
    applyToolbarStyle();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainView::applyToolbarStyle);

    // Prepare settings action icon and tooltip (will be positioned on the far right in setTitleWidget)
    if (m_settingsAction) {
        QIcon settingsIcon(":/resources/icons/custom/menu.svg");
        if (settingsIcon.isNull()) {
            settingsIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
        }
        m_settingsAction->setIcon(settingsIcon);
        m_settingsAction->setToolTip("Settings");
        m_settingsAction->setIconVisibleInMenu(true);
        // Connection already exists from setupMenus; no need to duplicate
    }
    
    // Connect the toggle button
    connect(m_toggleSidebarBtn, &QPushButton::toggled, this, &MainView::toggleSidebar);

    // Wrap toolbar in a scroll area to mirror TextEditor overflow behaviour
    if (!m_toolbarArea) {
        m_toolbarArea = new QScrollArea(this);
        m_toolbarArea->setFrameShape(QFrame::NoFrame);
        m_toolbarArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_toolbarArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_toolbarArea->setWidgetResizable(false);
        m_toolbarArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_toolbarArea->setWidget(m_toolbar);
        m_toolbarArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_toolbarArea->viewport(), QScroller::TouchGesture);

        // Determine fixed toolbar height deterministically (match TextEditor) so we reserve the exact
        // vertical space for the horizontal scrollbar and avoid any layout-driven height jitter.
        Theme theme = ThemeManager::instance()->currentTheme();
        const int toolBtnHeight = m_toolbar->iconSize().height();
        const int toolbarVPadding = 8;
        // Use touchTarget to ensure touch-friendly height; mirror applyToolbarStyle()
        int toolbarFixedH = qMax(toolBtnHeight, theme.metrics.touchTarget) + toolbarVPadding;
        const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_toolbarArea);
        const int extraPad = 2;
            // Reserve exact vertical space equal to toolbar + scrollbar to prevent jiggle
            m_toolbarArea->setFixedHeight(toolbarFixedH + scrollBarExtent + extraPad);
            // Use the global application stylesheet (ThemeManager) for scrollbar visuals so
            // scrollbars remain consistent across the app. Do not set a per-widget stylesheet here.

        // Wire scrollbar changes to update overscroll indicators
        if (m_toolbarArea->horizontalScrollBar()) {
            connect(m_toolbarArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, &MainView::updateOverscrollIndicators);
            connect(m_toolbarArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &MainView::updateOverscrollIndicators);
        }
    }

    // Create an affixed settings button on the far right
    if (!m_settingsBtn && m_settingsAction) {
        Theme theme = ThemeManager::instance()->currentTheme();
        m_settingsBtn = new QToolButton(this);
        m_settingsBtn->setDefaultAction(m_settingsAction);
        m_settingsBtn->setAutoRaise(true);
        m_settingsBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        // Make settings button narrower to give titlebar more room
        int settingsWidth = theme.metrics.iconSize + 12; // icon + 6px padding each side
        m_settingsBtn->setFixedSize(settingsWidth, theme.metrics.touchTarget);
        // Apply initial and theme-updated settings button styling via member function
        applySettingsStyle();
        connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainView::applySettingsStyle);
    }

    // Create overscroll indicators similar to TextEditor
    if (!m_overscrollLeftWidget) {
        m_overscrollLeftWidget = new QToolButton(this);
        m_overscrollLeftWidget->setIcon(QIcon(":/resources/icons/custom/chevrons-left.svg"));
        m_overscrollLeftWidget->setVisible(false);
        connect(m_overscrollLeftWidget, &QToolButton::clicked, this, &MainView::scrollToolbarLeft);
    }

    if (!m_overscrollRightWidget) {
        m_overscrollRightWidget = new QToolButton(this);
        m_overscrollRightWidget->setIcon(QIcon(":/resources/icons/custom/chevrons-right.svg"));
        m_overscrollRightWidget->setVisible(false);
        connect(m_overscrollRightWidget, &QToolButton::clicked, this, &MainView::scrollToolbarRight);
    }

    // Ensure toggle/settings and overscroll widgets honor styled backgrounds
    if (m_toggleSidebarBtn) {
        m_toggleSidebarBtn->setAttribute(Qt::WA_StyledBackground, true);
        m_toggleSidebarBtn->setAutoFillBackground(true);
    }
    if (m_settingsBtn) {
        m_settingsBtn->setAttribute(Qt::WA_StyledBackground, true);
        m_settingsBtn->setAutoFillBackground(true);
    }
    if (m_overscrollLeftWidget) {
        m_overscrollLeftWidget->setAttribute(Qt::WA_StyledBackground, true);
        m_overscrollLeftWidget->setAutoFillBackground(true);
    }
    if (m_overscrollRightWidget) {
        m_overscrollRightWidget->setAttribute(Qt::WA_StyledBackground, true);
        m_overscrollRightWidget->setAutoFillBackground(true);
    }

    // Apply overlay style to overscroll indicators to match TextEditor
    applyOverlayStyleToMain();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainView::applyOverlayStyleToMain);

    // Create a single row container that holds: left-fixed (toggle + title), scrollable toolbar, and fixed settings button
    m_toolbarRow = new QWidget(this);
    m_toolbarRow->setObjectName("ToolbarRow");
    m_toolbarRow->setAttribute(Qt::WA_StyledBackground, true);
    m_toolbarRow->setAutoFillBackground(true);
    QHBoxLayout *rowLayout = new QHBoxLayout(m_toolbarRow);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(0);

    // Left fixed area (toggle + title)
    m_toolbarLeftFixed = new QWidget(m_toolbarRow);
    m_toolbarLeftFixed->setObjectName("ToolbarLeftFixed");
    m_toolbarLeftFixed->setAttribute(Qt::WA_StyledBackground, true);
    m_toolbarLeftFixed->setAutoFillBackground(true);
    QHBoxLayout *leftLayout = new QHBoxLayout(m_toolbarLeftFixed);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);
    // Align children to vertical center so toggle and title line up with toolbar
    leftLayout->setAlignment(Qt::AlignVCenter);
    leftLayout->addWidget(m_toggleSidebarBtn);
    // Title widget will be inserted into m_toolbarLeftFixed by setTitleWidget
    rowLayout->addWidget(m_toolbarLeftFixed);

    // Scrollable toolbar area in the center
    if (m_toolbarArea) {
        m_toolbarArea->setObjectName("ToolbarArea");
        m_toolbarArea->setAttribute(Qt::WA_StyledBackground, true);
        if (m_toolbarArea->viewport()) {
            m_toolbarArea->viewport()->setAttribute(Qt::WA_StyledBackground, true);
            m_toolbarArea->viewport()->setAutoFillBackground(true);
        }
    }
    rowLayout->addWidget(m_toolbarArea, 1);

    // Fixed settings button on the right — align to vertical center so it lines up with toolbar
    if (m_settingsBtn) {
        rowLayout->addWidget(m_settingsBtn, 0, Qt::AlignVCenter);
    }

    // Insert combined toolbar row at the top of the main layout
    m_mainLayout->insertWidget(0, m_toolbarRow);
}

void MainView::setTitleWidget(QWidget *widget)
{
    if (!m_toolbar || !widget)
        return;

    // Store reference to title bar widget
    m_titleBarWidget = qobject_cast<TitleBarWidget*>(widget);

    // Remove previous title if it exists in the left fixed area
    if (m_titleBarWidget && m_toolbarLeftFixed) {
        // Remove any existing child title widgets
        QList<QWidget*> children = m_toolbarLeftFixed->findChildren<QWidget*>();
        for (QWidget* c : children) {
            if (c != m_toggleSidebarBtn) {
                c->deleteLater();
            }
        }
    }

    // Insert widget into left fixed area (next to toggle)
    widget->setParent(m_toolbarLeftFixed ? m_toolbarLeftFixed : m_toolbarRow);
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // Use touchTarget for height so it scales with zoom properly
    Theme theme = ThemeManager::instance()->currentTheme();
    widget->setFixedHeight(theme.metrics.touchTarget);
    // Ensure the title widget is wide enough to show at least ~10 characters on narrow screens
    // but do not override an explicit min/max width the widget may have already set
    // (for example, `TitleBarWidget` sets a width based on the theme touch target).
    if (widget->minimumWidth() == 0 && widget->maximumWidth() == QWIDGETSIZE_MAX) {
        QFontMetrics fm(widget->font());
        int minW = fm.horizontalAdvance(QString(10, QChar('M')));
        // Reduce the previously large minimum width to one third so the title area is less wide on phones
        int targetW = qMax(40, (minW + 16) / 3);
        widget->setMinimumWidth(targetW);
        // Also cap the maximum so the left area doesn't consume excessive space
        widget->setMaximumWidth(qMax(targetW, targetW * 3));
    }
    if (m_toolbarLeftFixed) {
        QHBoxLayout *leftLayout = qobject_cast<QHBoxLayout*>(m_toolbarLeftFixed->layout());
        if (leftLayout) leftLayout->addWidget(widget);
    }

    // Constrain the left-fixed area so it doesn't expand to fill the toolbar row.
    // Give more room to accommodate the larger titlebar widget.
    if (m_toolbarLeftFixed && m_toggleSidebarBtn) {
        int toggleW = m_toggleSidebarBtn->width() ? m_toggleSidebarBtn->width() : m_toggleSidebarBtn->sizeHint().width();
        int leftSpacing = 20; // increased spacing to prevent titlebar right edge clipping
        int leftMax = toggleW + widget->maximumWidth() + leftSpacing;
        m_toolbarLeftFixed->setMaximumWidth(leftMax);
        m_toolbarLeftFixed->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    }

    // Ensure TitleBarWidget (if provided) knows about ThemeManager so it paints
    // using the theme's menuBackground rather than its default palette.
    if (m_titleBarWidget) {
        m_titleBarWidget->setThemeManager(ThemeManager::instance());
    }

    // Activate layout
    if (layout()) layout()->activate();

    // Settings action is represented by the fixed `m_settingsBtn` on the right
    // (created in setupToolbar). Do not add the action to the scrollable
    // toolbar to avoid duplicating the control.
}

void MainView::removeToolbarWidget(QWidget *widget)
{
    if (!m_toolbar || !widget)
        return;

    // Find the QAction associated with this widget and remove it
    const auto acts = m_toolbar->actions();
    for (QAction *act : acts) {
        if (m_toolbar->widgetForAction(act) == widget) {
            m_toolbar->removeAction(act);
            // Schedule widget for deletion to avoid leaks
            widget->deleteLater();
            break;
        }
    }
}

void MainView::updateStatusBar(const QString &message, int timeout)
{
    QMainWindow *mainWindow = qobject_cast<QMainWindow*>(parentWidget());
    if (mainWindow && mainWindow->statusBar()) {
        mainWindow->statusBar()->showMessage(message, timeout);
    }
}

void MainView::setRootDirectory(const QString &path)
{
    m_rootDirectory = path;
    m_fileBrowser->setRootDirectory(path);
    
    // Also update the text editor's default save directory
    if (m_textEditor) {
        m_textEditor->setDefaultSaveDirectory(path);
    }
}

void MainView::toggleSidebar(bool visible)
{
    if (m_sidebarVisible == visible) {
        return; // No change needed
    }
    m_sidebarVisible = visible;
    
    updateStatusBar(visible ? "Showing sidebar..." : "Hiding sidebar...", 1000);
    
    if (!m_splitter || !m_sidebar || !m_textEditor) {
        return; // Safety check
    }
    
    if (visible) {
        // Show sidebar - restore its visibility
        m_sidebar->show();
        
        // Restore previous sidebar width, or use default
        int sidebarWidth = (m_sidebarWidth > 0) ? m_sidebarWidth : 250;
        int totalWidth = m_splitter->width();
        int editorWidth = totalWidth - sidebarWidth;
        
        // Ensure positive widths
        if (editorWidth < 100) {
            editorWidth = 100;
            sidebarWidth = totalWidth - editorWidth;
        }
        
        m_splitter->setSizes({sidebarWidth, editorWidth});
    } else {
        // Save current sidebar width before hiding
        QList<int> sizes = m_splitter->sizes();
        if (sizes.size() >= 2 && sizes[0] > 0) {
            m_sidebarWidth = sizes[0];
        }
        
        // Hide sidebar and give all space to editor
        m_sidebar->hide();
        
        // Force the splitter to give all space to the text editor
        int totalWidth = m_splitter->width();
        m_splitter->setSizes({0, totalWidth});
    }
    
    // Update toggle button state and icon if it exists
    if (m_toggleSidebarBtn) {
        m_toggleSidebarBtn->setChecked(visible);
        // Use chevrons-left when sidebar is visible (to hide it), chevrons-right when hidden (to show it)
        QIcon toggleIcon(visible ? ":/resources/icons/custom/chevrons-left.svg" 
                                 : ":/resources/icons/custom/chevrons-right.svg");
        if (!toggleIcon.isNull()) {
            m_toggleSidebarBtn->setIcon(toggleIcon);
        }
    }
}

void MainView::recomputeToolbarContentWidth()
{
    if (!m_toolbar) return;
    int contentWidth = 0;
    const int actionSpacing = 6;
    for (QAction* action : m_toolbar->actions()) {
        QWidget* w = m_toolbar->widgetForAction(action);
        if (w) {
            contentWidth += w->sizeHint().width() + actionSpacing;
        } else {
            // approximate width for separators or actions without widgets
            contentWidth += m_toolbar->iconSize().width() + actionSpacing;
        }
    }
    // Subtract the width of one item so the scrollable area appears slightly
    // shorter than the content (prevents a tiny visual overflow/gap).
    int reduceBy = 0;
    const auto acts = m_toolbar->actions();
    if (!acts.isEmpty()) {
        QAction* firstAct = acts.first();
        QWidget* w = m_toolbar->widgetForAction(firstAct);
        if (w) reduceBy = w->sizeHint().width() + actionSpacing;
        else reduceBy = m_toolbar->iconSize().width() + actionSpacing;
    }

    // Apply padding then reduce, clamping to a sane minimum
    contentWidth += 16; // padding
    contentWidth = qMax(0, contentWidth - reduceBy);
    m_toolbar->setFixedWidth(contentWidth);
    m_toolbar->adjustSize();
    if (m_toolbarArea) {
        m_toolbarArea->update();
        m_toolbarArea->viewport()->update();
    }
}

void MainView::applyToolbarStyle()
{
    if (!m_toolbar) return;
    Theme theme = ThemeManager::instance()->currentTheme();

    // Update toolbar icon size from theme
    m_toolbar->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
    
    // Update toggle button sizing
    if (m_toggleSidebarBtn) {
        m_toggleSidebarBtn->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
        // Make toggle narrower: icon width + small padding, but full touchTarget height
        int toggleWidth = theme.metrics.iconSize + 12; // icon + 6px padding each side
        m_toggleSidebarBtn->setFixedSize(toggleWidth, theme.metrics.touchTarget);
    }
    
    // Update settings button sizing - make narrower to give titlebar more room
    if (m_settingsBtn) {
        m_settingsBtn->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
        // Make settings button narrower: icon width + small padding, but full touchTarget height
        int settingsWidth = theme.metrics.iconSize + 12; // icon + 6px padding each side
        m_settingsBtn->setFixedSize(settingsWidth, theme.metrics.touchTarget);
    }

    // Compute a toolbar height that accommodates icons and the touch target metric
    const int toolBtnHeight = m_toolbar->iconSize().height();
    const int toolbarVPadding = 8;
    int toolbarFixedH = qMax(toolBtnHeight, theme.metrics.touchTarget) + toolbarVPadding;
    m_toolbar->setFixedHeight(toolbarFixedH);

    // Determine colors from ThemeManager so toolbar follows the active theme
    QColor menuBg = theme.colors.menuBackground.isValid() ? theme.colors.menuBackground : theme.colors.background.darker(110);
    QColor checkedBg = theme.colors.clicked.isValid() ? theme.colors.clicked : theme.colors.accent.darker(140);
    QColor checkedHover = checkedBg.lighter(110);
    // Prefer explicit toolbarTextIcon color from ThemeManager when available
    QColor textColor = theme.colors.toolbarTextIcon.isValid() ? theme.colors.toolbarTextIcon
                                                               : ((menuBg.lightness() < 128) ? QColor("#ffffff") : theme.colors.text);

    // Derive a slightly darker plate color for fixed components (toggle, titleplate, settings)
    QColor plateBg = theme.colors.menuBackground.isValid() ? theme.colors.menuBackground.darker(110)
                                                            : theme.colors.background.darker(130);

    // Let ThemeManager drive toolbar visuals via application stylesheet.
    // Avoid setting a widget-local stylesheet here because that would override
    // the application stylesheet and prevent unified theming.

    // Ensure no temporary debug borders remain on the toolbar.

    // Ensure the toolbar row and left-fixed area share the same darker plate so
    // fixed controls visually match the toolbar buttons.
    if (m_toolbarRow) {
        // Allow the application stylesheet to paint the toolbar row background
        // (ThemeManager targets `#ToolbarRow`) rather than relying on widget
        // palettes which can become stale. Ensure styled backgrounds are enabled.
        m_toolbarRow->setAttribute(Qt::WA_StyledBackground, true);
        m_toolbarRow->setAutoFillBackground(false);
        m_toolbarRow->update();
    }
    if (m_toolbarLeftFixed) {
        m_toolbarLeftFixed->setAttribute(Qt::WA_StyledBackground, true);
        m_toolbarLeftFixed->setAutoFillBackground(false);
        m_toolbarLeftFixed->update();
    }

    // Ensure the scroll area (center) also uses the same background so there is no
    // visible lighter strip behind the fixed components when the toolbar is scrolled.
    if (m_toolbarArea) {
        // The scroll area viewport is targeted by ThemeManager via objectName
        // `ToolbarArea` so it will receive the plate background from the
        // application stylesheet. Keep palette consistent as a fallback.
        if (m_toolbarArea->viewport()) {
            m_toolbarArea->viewport()->setAttribute(Qt::WA_StyledBackground, true);
            m_toolbarArea->viewport()->setAutoFillBackground(false);
            m_toolbarArea->viewport()->update();
        }
    // Restore normal scrollbar policy (allow Qt to show it as needed)
    m_toolbarArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }

    // Ensure title widget (if present) uses toolbar text/icon color
    if (m_titleBarWidget) {
        m_titleBarWidget->setStyleSheet(QString("color: %1;").arg(textColor.name()));
    }

    // Ensure the settings button (right fixed area) updates its palette so it
    // doesn't retain colors from the previous theme when themes switch.
    if (m_settingsBtn) {
        m_settingsBtn->setAttribute(Qt::WA_StyledBackground, true);
        m_settingsBtn->setAutoFillBackground(false);
        m_settingsBtn->update();
    }

    // Update scroll area scrollbar style to remain theme-aware
    if (m_toolbarArea) {
        const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_toolbarArea);
        const int extraPad = 2;
        // Height must reserve space for the horizontal scrollbar — visuals are styled globally
        m_toolbarArea->setFixedHeight(m_toolbar->height() + scrollBarExtent + extraPad);
    }

    // Force repaint of toolbar row and left-fixed area so any custom-painted
    // regions immediately reflect the new palette/styles.
    if (m_toolbarRow) m_toolbarRow->update();
    if (m_toolbarLeftFixed) m_toolbarLeftFixed->update();
    if (m_toolbarArea && m_toolbarArea->viewport()) m_toolbarArea->viewport()->update();
}

void MainView::updateOverscrollIndicators()
{
#ifdef Q_OS_ANDROID
    if (!m_toolbarArea || !m_toolbar || !m_overscrollLeftWidget || !m_overscrollRightWidget) return;

    QScrollBar* hbar = m_toolbarArea->horizontalScrollBar();
    if (!hbar) return;

    bool hasOverflow = (hbar->maximum() > hbar->minimum());
    bool canScrollLeft = hasOverflow && (hbar->value() > hbar->minimum());
    bool canScrollRight = hasOverflow && (hbar->value() < hbar->maximum());

    QRect viewportRect = m_toolbarArea->viewport()->rect();
    QPoint areaPos = m_toolbarArea->mapTo(this, QPoint(0, 0));

    if (canScrollLeft) {
        int leftX = areaPos.x() + 4;
        int leftY = areaPos.y() + (viewportRect.height() - m_overscrollLeftWidget->height()) / 2;
        m_overscrollLeftWidget->move(leftX, leftY);
        m_overscrollLeftWidget->raise();
        m_overscrollLeftWidget->setEnabled(true);
        m_overscrollLeftWidget->show();
    } else {
        m_overscrollLeftWidget->hide();
    }

    if (canScrollRight) {
        int rightX = areaPos.x() + viewportRect.width() - m_overscrollRightWidget->width() - 4;
        int rightY = areaPos.y() + (viewportRect.height() - m_overscrollRightWidget->height()) / 2;
        m_overscrollRightWidget->move(rightX, rightY);
        m_overscrollRightWidget->raise();
        m_overscrollRightWidget->setEnabled(true);
        m_overscrollRightWidget->show();
    } else {
        m_overscrollRightWidget->hide();
    }

    if (hasOverflow) QTimer::singleShot(100, this, [this]() { updateOverscrollIndicators(); });
#endif
}

void MainView::scrollToolbarLeft()
{
#ifdef Q_OS_ANDROID
    if (!m_toolbarArea) return;
    QScroller* scroller = QScroller::scroller(m_toolbarArea->viewport());
    if (scroller) {
        QPointF currentPos = scroller->finalPosition();
        QPointF newPos(currentPos.x() - 120, currentPos.y());
        scroller->scrollTo(newPos, 250);
        QTimer::singleShot(260, this, [this]() { updateOverscrollIndicators(); });
    }
#endif
}

void MainView::scrollToolbarRight()
{
#ifdef Q_OS_ANDROID
    if (!m_toolbarArea || !m_toolbar) return;
    QScroller* scroller = QScroller::scroller(m_toolbarArea->viewport());
    if (scroller) {
        QPointF currentPos = scroller->finalPosition();
        // compute content width
        int contentWidth = 0;
        for (QAction* action : m_toolbar->actions()) {
            QWidget* w = m_toolbar->widgetForAction(action);
            if (w) contentWidth += w->width() + 2;
            else contentWidth += 8;
        }
        contentWidth += 16;
        QRect viewportRect = m_toolbar->rect();
        int maxScroll = qMax(0, contentWidth - viewportRect.width());
        int targetX = qMin(static_cast<int>(currentPos.x() + 120), maxScroll);
        QPointF newPos(targetX, currentPos.y());
        scroller->scrollTo(newPos, 250);
        QTimer::singleShot(260, this, [this]() { updateOverscrollIndicators(); });
    }
#endif
}

void MainView::saveFile()
{
    // Update status bar
    updateStatusBar("Saving file...", 1000);
    
    // Check if text editor exists
    if (!m_textEditor) {
        updateStatusBar("Error: Text editor not available", 2000);
        return;
    }
    
    if (m_currentFile.isEmpty()) {
        // For new files, create filename from title bar and save to notes directory
        QString filename;
        if (m_titleBarWidget) {
            filename = m_titleBarWidget->filename();
        }
        
        // If no filename from title bar, use a default name
        if (filename.isEmpty()) {
            filename = "untitled.txt";
        }
        
        // Ensure we have a proper extension
        if (!filename.endsWith(".txt", Qt::CaseInsensitive) && 
            !filename.endsWith(".html", Qt::CaseInsensitive)) {
            filename += ".txt";
        }
        
        // Save to notes directory
        QString fullPath = m_rootDirectory + "/" + filename;
        
        // Ensure directory exists
        QDir().mkpath(m_rootDirectory);
        
        // Confirm overwrite if the file already exists
        if (QFileInfo::exists(fullPath)) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                tr("Overwrite File"),
                tr("'%1' already exists. Do you want to overwrite it?").arg(filename),
                QMessageBox::Yes | QMessageBox::No
            );
            if (reply != QMessageBox::Yes) {
                updateStatusBar("Save cancelled", 1500);
                return;
            }
        }
        
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_textEditor->getContent().toUtf8());
            file.close();
            m_currentFile = fullPath;
            m_textEditor->setFilePath(fullPath);
            m_textEditor->setModified(false);
            // Reflect saved name in the title bar widget
            if (m_titleBarWidget) {
                QFileInfo fi(fullPath);
                m_titleBarWidget->setFilename(fi.fileName());
            }
            updateWindowTitle();
            emit fileSaved(m_currentFile);
            
            // Update status bar
            updateStatusBar("File saved: " + filename, 2000);
        } else {
            // Update status bar with error
            updateStatusBar("Failed to save file: " + filename, 2000);
        }
    } else {
        m_textEditor->saveDocument();
        emit fileSaved(m_currentFile);
        
        // Update status bar
        QFileInfo fi(m_currentFile);
        updateStatusBar("File saved: " + fi.fileName(), 2000);
    }
}

bool MainView::promptSaveIfModified()
{
    // If no text editor or no modifications, it's safe to proceed
    if (!m_textEditor || !m_textEditor->isModified()) {
        return true;
    }
    
    // Determine the filename for the prompt
    QString filename;
    if (!m_currentFile.isEmpty()) {
        QFileInfo info(m_currentFile);
        filename = info.fileName();
    } else {
        filename = "Untitled";
    }
    
    // Ask user what to do
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Unsaved Changes"),
        tr("'%1' has been modified. Do you want to save your changes?").arg(filename),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save  // Default button
    );
    
    if (reply == QMessageBox::Save) {
        // Save the file
        saveFile();
        // Check if save was successful (file might still be modified if save failed)
        return !m_textEditor->isModified();
    } else if (reply == QMessageBox::Discard) {
        // User wants to discard changes
        return true;
    } else {
        // User cancelled
        return false;
    }
}

void MainView::loadFile(const QString &filePath)
{
    if (!m_textEditor) return;
    
    // Check if current file has unsaved changes
    if (!promptSaveIfModified()) {
        return; // User cancelled the operation
    }
    
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        m_textEditor->setContent(QString::fromUtf8(data));
        file.close();
        
        m_currentFile = filePath;
        m_textEditor->setFilePath(filePath);
        m_textEditor->setModified(false);
        // Update the title bar with the selected file's name
        if (m_titleBarWidget) {
            QFileInfo info(filePath);
            m_titleBarWidget->setFilename(info.fileName());
        }
        updateWindowTitle();
        
        emit fileOpened(filePath);
    }
}

void MainView::newFile()
{
    // Check if current file has unsaved changes
    if (!promptSaveIfModified()) {
        return; // User cancelled the operation
    }
    
    // Clear the current file path and content
    m_currentFile.clear();
    
    // Clear the text editor content
    if (m_textEditor) {
        m_textEditor->setContent("");
        m_textEditor->setFilePath("");
        m_textEditor->setModified(false);
    }
    
    // Update the window title
    updateWindowTitle();
    
    // Update status bar
    updateStatusBar("New document created", 2000);
}

void MainView::showSettings()
{
    emit settingsRequested();
}

void MainView::onFileSelected(const QString &filePath)
{
    QFileInfo info(filePath);
    const QString resolvedPath = info.absoluteFilePath();

    if (resolvedPath.isEmpty()) {
        updateStatusBar(tr("Unable to open file: missing path"), 4000);
        return;
    }

    info.setFile(resolvedPath);

    // Skip directories and divider files
    if (info.isDir() || info.suffix().compare("divider", Qt::CaseInsensitive) == 0) {
        return;
    }

    loadFile(resolvedPath);
}

void MainView::onFileSaved(const QString &filePath)
{
    m_currentFile = filePath;
    updateWindowTitle();
    emit fileSaved(filePath);

    // Refresh file browser to show any changes
    m_fileBrowser->populateTree();
}

void MainView::onEditorModified(bool modified)
{
    updateWindowTitle();
}

void MainView::updateWindowTitle()
{
    QString title = "QuteNote";

    if (!m_currentFile.isEmpty()) {
        QFileInfo info(m_currentFile);
        title += " - " + info.fileName();

        if (m_textEditor && m_textEditor->isModified()) {
            title += " *";
        }
    } else if (m_textEditor && m_textEditor->isModified()) {
        title += " - Untitled *";
    }

    if (parentWidget() && parentWidget()->window()) {
    #ifndef Q_OS_ANDROID
    parentWidget()->window()->setWindowTitle(title);
    #endif
    }
}
