#include "mainview.h"
#include <QtCore/qglobal.h>
#include "texteditor.h"
#include "filebrowser.h"
#include "settingsview.h"
#include "thememanager.h"
#include "titlebarwidget.h"

#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QTimer>
#include "uiutils.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QProgressBar>
#include <QPushButton>
#include <QToolBar>
#include <QAction>
#include <QStatusBar>
#include <QPropertyAnimation>
#include <QVariantAnimation>
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
#include <QEvent>
#include <QScrollArea>
#include <QScroller>
#include <QScrollBar>
#include <QFontMetrics>

MainView::MainView(QWidget *parent)
    : QWidget(parent)
    , m_rootDirectory(UIUtils::quteSettings().value("notesDirectory", UIUtils::defaultNotesDirectory()).toString())
    , m_currentFile(QString())
{
    // Set window properties for better mobile experience
    UIUtils::setupDesktopWindow(this, "QuteNote", 360, 600, 0, 0);

    // Enable touch events and gestures
    setAttribute(Qt::WA_AcceptTouchEvents);

    // Create main layout first
    m_mainLayout = QuteNote::makeOwned<QVBoxLayout>(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Setup UI components
    setupUI();
    setupToolbar();

    // Connect signals
    if (m_fileBrowser) {
        connect(m_fileBrowser.get(), &FileBrowser::fileSelected,
                this, &MainView::onFileSelected);
    }
    
    if (m_textEditor) {
        connect(m_textEditor.get(), &TextEditor::fileSaved,
                this, &MainView::onFileSaved);
        connect(m_textEditor.get(), &TextEditor::modificationChanged,
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
        ThemeManager::instance()->applyThemeToEditor(m_textEditor.get(), ThemeManager::instance()->editorTheme());
    }
    
    // Apply theme to file browser if it exists
    if (m_fileBrowser) {
        ThemeManager::instance()->applyThemeToFileBrowser(m_fileBrowser.get());
    }

    // Apply theme to panel handles
    if (m_panelManager) {
        m_panelManager->applyHandleStyle(m_panelManager->leftHandle(), PanelManager::PanelSide::Left);
        m_panelManager->applyHandleStyle(m_panelManager->rightHandle(), PanelManager::PanelSide::Right);
        m_panelManager->positionHandles();
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

    setBtn(m_overscrollLeftWidget.get());
    setBtn(m_overscrollRightWidget.get());

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

    // Detect Orientation changes (width vs height)
    int width = event->size().width();
    int height = event->size().height();
    bool isPortraitNow = (width < height);

    // Only process state transition if orientation actually changed
    if (isPortraitNow != m_isPortrait) {
        m_isPortrait = isPortraitNow;

        // Reset user manual-toggle tracker upon rotation change to give
        // the system control until they touch it again in this new state.
        setUserToggledSidebar(false);

        bool autoHideEnabled = UIUtils::quteSettings().value("autoHideSidebar", true).toBool();

        if (autoHideEnabled) {
            if (m_isPortrait) {
                // Entering Portrait: Save previous state, enforce hidden sidebar
                bool leftOpen = isPanelOpen(PanelManager::PanelSide::Left);
                m_wasSidebarVisibleBeforePortrait = leftOpen;
                if (leftOpen) {
                    toggleSidebar(false);
                    setUserToggledSidebar(false); // System generated this toggle
                }
            } else {
                // Entering Landscape: Restore if it was visible before portrait
                if (m_wasSidebarVisibleBeforePortrait && !isPanelOpen(PanelManager::PanelSide::Left)) {
                    toggleSidebar(true);
                    setUserToggledSidebar(false); // System generated this toggle
                }
            }
        }
    }
    
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

    // Reposition panel edge handles
    if (m_panelManager) m_panelManager->positionHandles();
    
    qDebug() << "[LayoutDebug] MainView target size:" << size();
    if (m_splitter) qDebug() << "[LayoutDebug] Splitter size:" << m_splitter->size();
    if (window()) qDebug() << "[LayoutDebug] Window size:" << window()->size();
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

void MainView::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::ApplicationFontChange) {
        recomputeToolbarContentWidth();
    }
    QWidget::changeEvent(event);
}

void MainView::setupUI()
{
    // ── Left sidebar (File Browser) ──────────────────────────────────────
    m_sidebar = QuteNote::makeOwned<QWidget>(this);
    m_sidebar->setMinimumWidth(0);
    m_sidebar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_fileBrowser = QuteNote::makeOwned<FileBrowser>(m_sidebar.get());
    m_fileBrowser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_sidebarLayout = QuteNote::makeOwned<QVBoxLayout>(m_sidebar.get());
    m_sidebarLayout->setContentsMargins(0, 0, 0, 0);
    m_sidebarLayout->setSpacing(0);
    m_sidebarLayout->addWidget(m_fileBrowser.get());

    // ── Center editor ────────────────────────────────────────────────────
    m_textEditor = QuteNote::makeOwned<TextEditor>(this);
    m_textEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // ── Right panel (Settings) ───────────────────────────────────────────
    m_rightPanel = QuteNote::makeOwned<QWidget>(this);
    m_rightPanel->setMinimumWidth(0);
    m_rightPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_rightPanelLayout = QuteNote::makeOwned<QVBoxLayout>(m_rightPanel.get());
    m_rightPanelLayout->setContentsMargins(0, 0, 0, 0);
    m_rightPanelLayout->setSpacing(0);

    m_settingsView = QuteNote::makeOwned<SettingsView>(m_rightPanel.get());
    m_settingsView->initializeComponent();
    m_settingsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_rightPanelLayout->addWidget(m_settingsView.get());

    // ── 3-panel splitter ─────────────────────────────────────────────────
    m_splitter = QuteNote::makeOwned<QSplitter>(Qt::Horizontal, this);
    m_splitter->setHandleWidth(8);
    m_splitter->setChildrenCollapsible(true);
    m_splitter->setOpaqueResize(true);
    m_splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_splitter->setFrameShape(QFrame::NoFrame);

    m_splitter->addWidget(m_sidebar.get());     // index 0
    m_splitter->addWidget(m_textEditor.get());  // index 1
    m_splitter->addWidget(m_rightPanel.get());  // index 2

    m_splitter->setStretchFactor(0, 0);  // Sidebar maintains its size
    m_splitter->setStretchFactor(1, 1);  // Editor stretches
    m_splitter->setStretchFactor(2, 0);  // Right panel maintains its size

    // Start with both panels closed
    m_splitter->setSizes({0, 1, 0});
    m_sidebar->hide();
    m_rightPanel->hide();

    m_mainLayout->addWidget(m_splitter.get());

    if (UIUtils::isMobileDevice()) {
        m_mainLayout->setStretch(0, 1);
    }

    // ── Panel manager (handles, animation, positioning) ───────────────────
    m_panelManager = QuteNote::makeOwned<PanelManager>(m_splitter.get(), m_sidebar.get(), m_rightPanel.get(), this);

    // ── Signal connections ───────────────────────────────────────────────
    connect(m_fileBrowser.get(), &FileBrowser::fileSelected,
            this, &MainView::onFileSelected);
    connect(m_textEditor.get(), &TextEditor::modificationChanged,
            this, &MainView::onEditorModified);

    // Settings view signals
    connect(m_settingsView.get(), &SettingsView::settingsChanged,
            this, &MainView::onSettingsChanged);
    connect(m_settingsView.get(), &SettingsView::backToMain,
            this, &MainView::onSettingsBackToMain);

    // Theme
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainView::onThemeChanged);
    connect(ThemeManager::instance(), &ThemeManager::themeApplyStarted, this, &MainView::onThemeApplyStarted);
    connect(ThemeManager::instance(), &ThemeManager::themeApplyFinished, this, &MainView::onThemeApplyFinished);
    onThemeChanged(ThemeManager::instance()->currentTheme());

    // ── Theme overlay ────────────────────────────────────────────────────
    m_themeOverlay = QuteNote::makeOwned<QWidget>(this);
    m_themeOverlay->setObjectName("ThemeOverlay");
    m_themeOverlay->setVisible(false);
    m_themeOverlay->setAttribute(Qt::WA_NoSystemBackground, false);
    m_themeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_themeOverlay->setStyleSheet("QWidget#ThemeOverlay { background: rgba(0,0,0,0.22); }");
    QVBoxLayout *overlayLayout = new QVBoxLayout(m_themeOverlay.get());
    overlayLayout->setContentsMargins(0,0,0,0);
    overlayLayout->addStretch();
    m_themeProgressBar = QuteNote::makeOwned<QProgressBar>(m_themeOverlay.get());
    m_themeProgressBar->setFixedWidth(240);
    m_themeProgressBar->setRange(0, 0);
    m_themeProgressBar->setTextVisible(false);
    overlayLayout->addWidget(m_themeProgressBar.get(), 0, Qt::AlignHCenter);
    overlayLayout->addStretch();
}

void MainView::setupToolbar()
{
    // ── Create actions directly (no menu bar) ──────────────────────────────────
    m_newAction = QuteNote::makeOwned<QAction>(tr("&New"), this);
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction.get(), &QAction::triggered, this, [this]() {
        updateStatusBar("Creating new document...", 1000);
        newFile();
    });

    m_openAction = QuteNote::makeOwned<QAction>(tr("&Open"), this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction.get(), &QAction::triggered, this, [this]() {
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

    m_saveAction = QuteNote::makeOwned<QAction>(tr("&Save"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction.get(), &QAction::triggered, this, &MainView::saveFile);

    m_settingsAction = QuteNote::makeOwned<QAction>(tr("&Settings"), this);
    connect(m_settingsAction.get(), &QAction::triggered, this, &MainView::showSettings);

    m_undoAction = QuteNote::makeOwned<QAction>(tr("&Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);

    m_redoAction = QuteNote::makeOwned<QAction>(tr("&Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);

    m_cutAction = QuteNote::makeOwned<QAction>(tr("Cu&t"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);

    m_copyAction = QuteNote::makeOwned<QAction>(tr("&Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);

    m_pasteAction = QuteNote::makeOwned<QAction>(tr("&Paste"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);

    // ── Toolbar widget setup ─────────────────────────────────────────────────
    m_toolbar = QuteNote::makeOwned<QToolBar>(this);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(32, 32));  // Match TextEditor toolbar size
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
    m_toggleSidebarBtn = QuteNote::makeOwned<QToolButton>(this);
    m_toggleSidebarBtn->setCheckable(true);
    m_toggleSidebarBtn->setChecked(true);
    m_toggleSidebarBtn->setIcon(sidebarIcon);
    m_toggleSidebarBtn->setToolTip("Toggle Sidebar");
    m_toggleSidebarBtn->setIconSize(QSize(32, 32));
    m_toggleSidebarBtn->setFixedSize(44, 44); // Slightly larger to fit icon
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
    m_newAction->setIcon(loadThemedIcon(":/resources/icons/custom/new-file.svg", QStyle::SP_FileIcon));
    
    m_openAction->setIcon(loadThemedIcon(":/resources/icons/custom/file.svg", QStyle::SP_DialogOpenButton));
    
    m_saveAction->setIcon(loadThemedIcon(":/resources/icons/custom/save.svg", QStyle::SP_DialogSaveButton));
    
    m_undoAction->setIcon(loadThemedIcon(":/resources/icons/custom/undo.svg", QStyle::SP_ArrowBack));
    connect(m_undoAction.get(), &QAction::triggered, this, [this]() {
        updateStatusBar("Undoing last action...", 1000);
        if (m_textEditor) m_textEditor->undo();
    });
    
    m_redoAction->setIcon(loadThemedIcon(":/resources/icons/custom/redo.svg", QStyle::SP_ArrowForward));
    connect(m_redoAction.get(), &QAction::triggered, this, [this]() {
        updateStatusBar("Redoing last action...", 1000);
        if (m_textEditor) m_textEditor->redo();
    });
    
    m_cutAction->setIcon(loadThemedIcon(":/resources/icons/custom/cut.svg", QStyle::SP_DialogCancelButton));
    connect(m_cutAction.get(), &QAction::triggered, this, [this]() {
        updateStatusBar("Cutting selected text...", 1000);
        if (m_textEditor) m_textEditor->cut();
    });
    
    m_copyAction->setIcon(loadThemedIcon(":/resources/icons/custom/copy.svg", QStyle::SP_CommandLink));
    connect(m_copyAction.get(), &QAction::triggered, this, [this]() {
        updateStatusBar("Copying selected text...", 1000);
        if (m_textEditor) m_textEditor->copy();
    });
    
    m_pasteAction->setIcon(loadThemedIcon(":/resources/icons/custom/paste.svg", QStyle::SP_DialogApplyButton));
    connect(m_pasteAction.get(), &QAction::triggered, this, [this]() {
        updateStatusBar("Pasting clipboard content...", 1000);
        if (m_textEditor) m_textEditor->paste();
    });
    
    // Add main actions with reduced spacing
    m_toolbar->addAction(m_newAction.get());
    m_toolbar->addAction(m_openAction.get());
    m_toolbar->addAction(m_saveAction.get());
    
    // Add separator with reduced width
    QWidget *separator2 = new QWidget();
    separator2->setFixedWidth(6);
    m_toolbar->addWidget(separator2);
    
    m_toolbar->addAction(m_undoAction.get());
    m_toolbar->addAction(m_redoAction.get());
    
    // Add separator with reduced width
    QWidget *separator3 = new QWidget();
    separator3->setFixedWidth(6);
    m_toolbar->addWidget(separator3);
    
    m_toolbar->addAction(m_cutAction.get());
    m_toolbar->addAction(m_copyAction.get());
    m_toolbar->addAction(m_pasteAction.get());

    // Finalize toolbar sizing so the scroll area will know when content overflows
    recomputeToolbarContentWidth();

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
    connect(m_toggleSidebarBtn.get(), &QPushButton::toggled, this, &MainView::toggleSidebar);

    // Wrap toolbar in a scroll area to mirror TextEditor overflow behaviour
    if (!m_toolbarArea) {
        m_toolbarArea = QuteNote::makeOwned<QScrollArea>(this);
        m_toolbarArea->setFrameShape(QFrame::NoFrame);
        m_toolbarArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_toolbarArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_toolbarArea->setWidgetResizable(false);
        m_toolbarArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_toolbarArea->setWidget(m_toolbar.get());
        m_toolbarArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        QScroller::grabGesture(m_toolbarArea->viewport(), QScroller::TouchGesture);

        // Determine fixed toolbar height deterministically (match TextEditor) so we reserve the exact
        // vertical space for the horizontal scrollbar and avoid any layout-driven height jitter.
        Theme theme = ThemeManager::instance()->currentTheme();
        const int toolBtnHeight = m_toolbar->iconSize().height();
        const int toolbarVPadding = 8;
        // Use touchTarget to ensure touch-friendly height; mirror applyToolbarStyle()
        int toolbarFixedH = qMax(toolBtnHeight, theme.metrics.touchTarget) + toolbarVPadding;
        const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_toolbarArea.get());
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
        m_settingsBtn = QuteNote::makeOwned<QToolButton>(this);
        m_settingsBtn->setDefaultAction(m_settingsAction.get());
        m_settingsBtn->setAutoRaise(true);
        m_settingsBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_settingsBtn->setFixedSize(44, 44);
        // Apply initial and theme-updated settings button styling via member function
        applySettingsStyle();
        connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainView::applySettingsStyle);
    }

    // Create overscroll indicators similar to TextEditor
    if (!m_overscrollLeftWidget) {
        m_overscrollLeftWidget = QuteNote::makeOwned<QToolButton>(this);
        m_overscrollLeftWidget->setIcon(QIcon(":/resources/icons/custom/chevrons-left.svg"));
        m_overscrollLeftWidget->setVisible(false);
        connect(m_overscrollLeftWidget.get(), &QToolButton::clicked, this, &MainView::scrollToolbarLeft);
    }

    if (!m_overscrollRightWidget) {
        m_overscrollRightWidget = QuteNote::makeOwned<QToolButton>(this);
        m_overscrollRightWidget->setIcon(QIcon(":/resources/icons/custom/chevrons-right.svg"));
        m_overscrollRightWidget->setVisible(false);
        connect(m_overscrollRightWidget.get(), &QToolButton::clicked, this, &MainView::scrollToolbarRight);
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
    m_toolbarRow = QuteNote::makeOwned<QWidget>(this);
    m_toolbarRow->setObjectName("ToolbarRow");
    m_toolbarRow->setAttribute(Qt::WA_StyledBackground, true);
    m_toolbarRow->setAutoFillBackground(true);
    QHBoxLayout *rowLayout = new QHBoxLayout(m_toolbarRow.get());
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(0);

    // Left fixed area (toggle + title)
    m_toolbarLeftFixed = QuteNote::makeOwned<QWidget>(m_toolbarRow.get());
    m_toolbarLeftFixed->setObjectName("ToolbarLeftFixed");
    m_toolbarLeftFixed->setAttribute(Qt::WA_StyledBackground, true);
    m_toolbarLeftFixed->setAutoFillBackground(true);
    QHBoxLayout *leftLayout = new QHBoxLayout(m_toolbarLeftFixed.get());
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);
    // Align children to the top so title lines up with toolbar
    leftLayout->setAlignment(Qt::AlignTop);
    // m_toggleSidebarBtn replaced by left panel edge handle — hide it
    m_toggleSidebarBtn->setVisible(false);
    // Title widget will be inserted into m_toolbarLeftFixed by setTitleWidget
    rowLayout->addWidget(m_toolbarLeftFixed.get());

    // Scrollable toolbar area in the center
    if (m_toolbarArea) {
        m_toolbarArea->setObjectName("ToolbarArea");
        m_toolbarArea->setAttribute(Qt::WA_StyledBackground, true);
        if (m_toolbarArea->viewport()) {
            m_toolbarArea->viewport()->setAttribute(Qt::WA_StyledBackground, true);
            m_toolbarArea->viewport()->setAutoFillBackground(true);
        }
    }
    rowLayout->addWidget(m_toolbarArea.get(), 1);

    // Fixed settings button replaced by right panel edge handle — hide it
    if (m_settingsBtn) {
        m_settingsBtn->setVisible(false);
    }

    // Insert combined toolbar row at the top of the main layout
    m_mainLayout->insertWidget(0, m_toolbarRow.get());
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
            if (c != m_toggleSidebarBtn.get()) {
                c->deleteLater();
            }
        }
    }

    // Insert widget into left fixed area (next to toggle)
    widget->setParent(m_toolbarLeftFixed ? m_toolbarLeftFixed.get() : m_toolbarRow.get());
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // Give a little extra vertical room on phone (Android) to avoid cramped titlebar
    int extraPhonePad = 0;
#ifdef Q_OS_ANDROID
    extraPhonePad = 8;
#endif
    widget->setFixedHeight(m_toolbar->iconSize().height() + 8 + extraPhonePad);
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
    if (m_toolbarLeftFixed && m_toggleSidebarBtn) {
        int toggleW = m_toggleSidebarBtn->width() ? m_toggleSidebarBtn->width() : m_toggleSidebarBtn->sizeHint().width();
        int leftSpacing = 8; // small padding
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

// ── Panel System ───────────────────────────────────────────────────────────

bool MainView::isPanelOpen(PanelManager::PanelSide side) const
{
    return m_panelManager ? m_panelManager->isPanelOpen(side) : false;
}

void MainView::toggleSidebar(bool visible)
{
    // Backward-compatible wrapper — delegates to the unified panel system
    togglePanel(PanelManager::PanelSide::Left, visible);
}

void MainView::togglePanel(PanelManager::PanelSide side, bool open)
{
    if (m_panelManager) m_panelManager->togglePanel(side, open);
}

// ── Icon helper ────────────────────────────────────────────────────────────

QIcon MainView::loadThemedIcon(const QString &resourcePath, QStyle::StandardPixmap fallback) const
{
    QIcon icon(resourcePath);
    if (icon.isNull()) {
        icon = style()->standardIcon(fallback);
    }
    return icon;
}

// ── Settings callbacks ────────────────────────────────────────────────────

void MainView::onSettingsChanged()
{
    // Save & Apply — persist settings, apply notes directory, close panel
    QSettings& settings = UIUtils::quteSettings();
    QString notesDir = settings.value("notesDirectory",
        UIUtils::defaultNotesDirectory()).toString();
    setRootDirectory(notesDir);
    togglePanel(PanelSide::Right, false);
}

void MainView::onSettingsBackToMain()
{
    // Back to Main — just close the settings panel
    togglePanel(PanelSide::Right, false);
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
        const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_toolbarArea.get());
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
    scrollToolbarBy(-120);
}

void MainView::scrollToolbarRight()
{
    scrollToolbarBy(120);
}

void MainView::scrollToolbarBy(int delta)
{
#ifdef Q_OS_ANDROID
    if (!m_toolbarArea || !m_toolbar) return;
    QScroller* scroller = QScroller::scroller(m_toolbarArea->viewport());
    if (!scroller) return;

    QPointF currentPos = scroller->finalPosition();
    int maxScroll = 0;

    // Compute content width for clamping
    int contentWidth = 0;
    for (QAction* action : m_toolbar->actions()) {
        QWidget* w = m_toolbar->widgetForAction(action);
        if (w) contentWidth += w->width() + 2;
        else contentWidth += 8;
    }
    contentWidth += 16;
    QRect viewportRect = m_toolbar->rect();
    maxScroll = qMax(0, contentWidth - viewportRect.width());

    int targetX = qBound(0, static_cast<int>(currentPos.x() + delta), maxScroll);
    QPointF newPos(targetX, currentPos.y());
    scroller->scrollTo(newPos, 250);
    QTimer::singleShot(260, this, [this]() { updateOverscrollIndicators(); });
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
    togglePanel(PanelSide::Right, true);
}

void MainView::onFileSelected(const QString &filePath)
{
    QFileInfo info(filePath);

    // Skip directories and divider files
    if (info.isDir() || info.suffix() == "divider") {
        return;
    }

    loadFile(filePath);
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
