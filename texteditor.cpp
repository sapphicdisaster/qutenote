#include "texteditor.h"
#include <QtCore/qglobal.h>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextList>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QTextListFormat>
#include <QTextImageFormat>
#include <QTextDocumentFragment>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDatabase>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QInputDialog>
#include <QUrl>
#include <QStandardPaths>
#include <QFile>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>
#include <qstyle.h>
#include <QLabel>
#include <QToolButton>
#include <QFontComboBox>
#include <QTimer>
#include <QApplication>
#include <QInputMethod>
#include <QScroller>
#include <QScrollerProperties>
#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "colorpicker.h"
#include "thememanager.h"
#include "uiutils.h"

TextEditor::TextEditor(QWidget *parent)
    : QuteNote::ComponentBase(parent)
    , m_filePath("")
    , m_modified(false)
{
    initializeComponent();
}

TextEditor::~TextEditor()
{
    // Cleanup will be handled by cleanupResources()
}

void TextEditor::initializeComponent()
{
    if (isInitialized()) {
        return;
    }
    
    // Set component name
    setComponentName("TextEditor");
    
#ifndef Q_OS_ANDROID
    // Initialize touch handler only for non-Android platforms, as it interferes
    // with native text selection and scrolling.
    m_touchHandler = QuteNote::makeOwned<TextEditorTouchHandler>(this);
#endif
    
    // Setup actions, UI and connections
    setupActions();
    setupMenus();
    setupUI();
    setupConnections();
    
    // Mark as initialized
    markInitialized();
    
#ifdef Q_OS_ANDROID
    // Refresh toolbar on Android to ensure proper visibility
    refreshToolbar();
    
    // Update overscroll indicators after a short delay to ensure toolbar is fully initialized
    QTimer::singleShot(100, this, &TextEditor::updateOverscrollIndicators);
#endif
    
    // Connect to theme changes
    connect(ThemeManager::instance(), &ThemeManager::editorThemeChanged, 
            this, [this](const EditorTheme &theme) {
        ThemeManager::instance()->applyThemeToEditor(this, theme);
    });
    
    // Apply initial theme
    ThemeManager::instance()->applyThemeToEditor(this, ThemeManager::instance()->editorTheme());
    
    // Call base implementation
    ComponentBase::initializeComponent();
}

void TextEditor::setupConnections()
{
    // Connect editor signals
    connect(m_editor.get(), &QTextEdit::textChanged, 
            this, &TextEditor::onTextChanged);
    connect(m_editor.get(), &QTextEdit::cursorPositionChanged, 
            this, &TextEditor::onCursorPositionChanged);
    
    // Connect action signals
    connect(m_boldAction.get(), &QAction::triggered, 
            this, &TextEditor::onBold);
    connect(m_italicAction.get(), &QAction::triggered, 
            this, &TextEditor::onItalic);
    connect(m_underlineAction.get(), &QAction::triggered, 
            this, &TextEditor::onUnderline);
    connect(m_strikeAction.get(), &QAction::triggered, 
            this, &TextEditor::onStrikethrough);
    
    connect(m_alignLeftAction.get(), &QAction::triggered, 
            this, &TextEditor::onAlignLeft);
    connect(m_alignCenterAction.get(), &QAction::triggered, 
            this, &TextEditor::onAlignCenter);
    connect(m_alignRightAction.get(), &QAction::triggered, 
            this, &TextEditor::onAlignRight);
    connect(m_alignJustifyAction.get(), &QAction::triggered, 
            this, &TextEditor::onAlignJustify);
    
    connect(m_bulletListAction.get(), &QAction::triggered, 
            this, &TextEditor::onBulletList);
    connect(m_numberedListAction.get(), &QAction::triggered, 
            this, &TextEditor::onNumberedList);
    
    connect(m_linkAction.get(), &QAction::triggered, 
            this, &TextEditor::onInsertLink);
    connect(m_imageAction.get(), &QAction::triggered, 
            this, &TextEditor::onInsertImage);
    
    connect(m_textColorAction.get(), &QAction::triggered, 
            this, &TextEditor::onTextColor);
    connect(m_bgColorAction.get(), &QAction::triggered, 
            this, &TextEditor::onBackgroundColor);
    
    // Connect font and size changes
    connect(m_fontCombo.get(), &QFontComboBox::currentFontChanged,
            this, &TextEditor::onFontChanged);
    connect(m_sizeCombo.get(), &QComboBox::currentTextChanged,
            this, &TextEditor::onFontSizeChanged);
    
    // Toolbar scroll events are connected in setupToolbar method
    
    // Call base implementation
    ComponentBase::setupConnections();
}

void TextEditor::cleanupResources()
{
    if (!isInitialized()) {
        return;
    }

    // Save document if modified
    if (m_modified && !m_filePath.isEmpty()) {
        if (QMessageBox::question(this, tr("Save Changes?"),
                tr("The document has been modified.\nDo you want to save your changes?"),
                QMessageBox::Save | QMessageBox::Discard) == QMessageBox::Save) {
            saveDocument();
        }
    }

    // Clear document content to free memory
    if (m_editor && m_editor->document()) {
        m_editor->document()->clear();
    }

    // Clean up touch handler
    m_touchHandler = nullptr;

}

void TextEditor::handleMemoryWarning()
{
    // On memory pressure:
    // 1. Clear undo/redo stacks
    if (m_editor && m_editor->document()) {
        m_editor->document()->clearUndoRedoStacks();
    }

    // 2. Consider simplifying document formatting
    // (could be implemented if needed)

    // 3. Notify user
    QMessageBox::warning(this, tr("Memory Warning"),
        tr("The document is using a large amount of memory.\nUndo history has been cleared to reduce memory usage."));
}

qreal TextEditor::zoomFactor() const
{
    // QTextEdit doesn't have zoomFactor(), use font size instead
    if (!m_editor) return 1.0;
    QFont font = m_editor->font();
    return font.pointSizeF() / 12.0; // Assuming 12pt is base size
}

void TextEditor::setZoomFactor(qreal factor)
{
    if (!m_editor) return;
    // QTextEdit doesn't have setZoomFactor(), use font size instead
    QFont font = m_editor->font();
    font.setPointSizeF(12.0 * factor); // Assuming 12pt is base size
    m_editor->setFont(font);
    emit zoomFactorChanged(factor);
}

QWidget* TextEditor::viewport() const
{
    return m_editor ? m_editor->viewport() : nullptr;
}

QTextDocument* TextEditor::document() const
{
    return m_editor ? m_editor->document() : nullptr;
}

QScrollBar* TextEditor::verticalScrollBar() const
{
    return m_editor ? m_editor->verticalScrollBar() : nullptr;
}

bool TextEditor::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        const QList<QEventPoint> &points = touchEvent->points();
        
        if (points.isEmpty()) {
            return QWidget::event(event);
        }
        
        QPoint touchPos = touchEvent->points().first().position().toPoint();
        
        // Check if touch is on overscroll indicator widgets first (they're overlays)
#ifdef Q_OS_ANDROID
        if (m_overscrollLeftWidget && m_overscrollLeftWidget->isVisible() && 
            m_overscrollLeftWidget->geometry().contains(touchPos)) {
            // Forward the touch event directly to the overscroll widget
            QTouchEvent* clonedEvent = new QTouchEvent(
                event->type(),
                touchEvent->pointingDevice(),
                touchEvent->modifiers(),
                touchEvent->points()
            );
            QApplication::postEvent(m_overscrollLeftWidget.get(), clonedEvent);
            return true; // Mark as handled
        }
        
        if (m_overscrollRightWidget && m_overscrollRightWidget->isVisible() && 
            m_overscrollRightWidget->geometry().contains(touchPos)) {
            // Forward the touch event directly to the overscroll widget
            QTouchEvent* clonedEvent = new QTouchEvent(
                event->type(),
                touchEvent->pointingDevice(),
                touchEvent->modifiers(),
                touchEvent->points()
            );
            QApplication::postEvent(m_overscrollRightWidget.get(), clonedEvent);
            return true; // Mark as handled
        }
#endif
        
        // If touch is on the toolbar, let it handle the event
        if (m_toolbar && m_toolbar->geometry().contains(touchPos)) {
            // Don't handle toolbar touches here, let them propagate to toolbar buttons
            return QWidget::event(event);
        }
        
        if (points.count() == 2) {
            // Handle two-finger gestures (pinch to zoom, etc.)
            return true;
        }
        
        // On Android, ensure editor focus when touching the editor area
#ifdef Q_OS_ANDROID
        if (event->type() == QEvent::TouchBegin) {
        }
#endif
        
        // Let the base class handle single touch events
        return QWidget::event(event);
    }
    case QEvent::KeyPress:
    {
        break;
    }
    case QEvent::Resize:
        // Ensure the editor properly resizes when the widget is resized
        if (m_editor) {
            m_editor->updateGeometry();
        }
        break;
    case QEvent::Gesture:
        return gestureEvent(static_cast<QGestureEvent*>(event));
    case QEvent::FocusIn:
        break;
    case QEvent::FocusOut:
        break;
    default:
        return QWidget::event(event);
    }
    return QWidget::event(event);
}

bool TextEditor::eventFilter(QObject *obj, QEvent *event)
{
    // Handle touch events for toolbar and toolbar buttons
    if (event->type() == QEvent::TouchBegin || 
        event->type() == QEvent::TouchUpdate || 
        event->type() == QEvent::TouchEnd) {
        
        // Check if the object is a toolbar button
        if (QToolButton *button = qobject_cast<QToolButton*>(obj)) {
            // Handle touch events for toolbar buttons
            if (event->type() == QEvent::TouchBegin) {
                // Don't manually trigger the action - let Qt handle it
                // Return false to let Qt handle the touch event normally
                return false;
            } else if (event->type() == QEvent::TouchEnd) {
                // Return false to let Qt handle the touch event normally
                return false;
            }
        }
        
        // Check if the object is the toolbar scroll area or its viewport
        if (m_toolbarArea && (obj == m_toolbarArea->viewport() || obj == m_toolbarArea.get())) {
            // Let the toolbar handle its own touch events (scrolling)
            // But handle touch begin specifically to ensure proper focus management
            if (event->type() == QEvent::TouchBegin) {
            }
            // Return false to let Qt handle the scrolling
            return false;
        }
    }
    
    // Handle resize events to ensure proper layout
    if (event->type() == QEvent::Resize) {
        // Force the editor to update its layout
        if (m_editor) {
            m_editor->updateGeometry();
        }
    }
    
    // Let the base class handle other events
    return ComponentBase::eventFilter(obj, event);
}

bool TextEditor::gestureEvent(QGestureEvent *event)
{
    if (QPinchGesture *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
        if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged) {
            // Handle pinch zoom
            return true;
        }
    }
    return true;
}

void TextEditor::resizeEvent(QResizeEvent *event)
{
    // Call the base class implementation first
    QWidget::resizeEvent(event);
    
    // Let Qt's layout system handle the resizing
    if (m_editor) {
        m_editor->updateGeometry();
    }
    
    if (layout()) {
        layout()->activate();
    }
}

void TextEditor::showEvent(QShowEvent *event)
{
    // Call the base class implementation first
    QWidget::showEvent(event);
    
    // Let Qt's layout system handle the layout
    if (m_editor) {
        m_editor->updateGeometry();
    }
    
    if (layout()) {
        layout()->activate();
    }
}

void TextEditor::setupUI()
{
    // Create editor and its container
    m_editorContainer = QuteNote::makeOwned<QWidget>(this);
    m_editor = QuteNote::makeOwned<QTextEdit>(m_editorContainer.get());
    m_editorContainer->setObjectName("editorContainer");
    
    // Disable drag-and-drop to prevent file browser items being dropped as paths
    m_editor->setAcceptDrops(false);

    // Create touch-optimized toolbar
    m_toolbar = QuteNote::makeOwned<QToolBar>(this);
    #ifndef Q_OS_ANDROID
    m_toolbar->setWindowTitle("Format");
    #endif
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    
    // Set icon size for better visibility
    m_toolbar->setIconSize(QSize(32, 32)); // Larger icons for better visibility
    
    // Toolbar sizing (content-sized width; height fixed)
    m_toolbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    // Ensure a stable, fixed toolbar height to avoid vertical bounce
    const int toolBtnHeight = 48;            // touch target for buttons
    const int toolbarVPadding = 8;           // extra vertical padding to lock height
    const int toolbarFixedH = toolBtnHeight + toolbarVPadding;
    m_toolbar->setFixedHeight(toolbarFixedH);
    
    // Ensure toolbar properly handles touch events
    m_toolbar->setAttribute(Qt::WA_AcceptTouchEvents, true);
    m_toolbar->setFocusPolicy(Qt::NoFocus); // Prevent toolbar from stealing focus
    
    // For Android, ensure proper toolbar configuration
#ifdef Q_OS_ANDROID
    // Connect overlay widgets directly to scroll actions
    connect(m_overscrollLeftWidget.get(), &QToolButton::clicked, this, &TextEditor::scrollToolbarLeft);
    connect(m_overscrollRightWidget.get(), &QToolButton::clicked, this, &TextEditor::scrollToolbarRight);
#endif

    // Build toolbar contents
    setupToolbar();
    setupEditor();

    // Wrap toolbar in a horizontal scroll area for proper overflow scrolling
    m_toolbarArea = QuteNote::makeOwned<QScrollArea>(this);
    m_toolbarArea->setFrameShape(QFrame::NoFrame);
    m_toolbarArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_toolbarArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_toolbarArea->setWidgetResizable(false); // Keep content width; allow horizontal scroll
    m_toolbarArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // Reserve exact space for the horizontal scrollbar under the toolbar, so there's no vertical scroll of buttons
    {
        const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_toolbarArea.get());
        const int extraPad = 2;  // match FileBrowser padding; kills 1-2px jiggle on some styles
        int effectiveToolbarH = m_toolbar->height();
        if (effectiveToolbarH <= 0) effectiveToolbarH = toolbarFixedH;
        // Reserve exact vertical space equal to the toolbar's actual height to avoid vertical bounce
        m_toolbarArea->setFixedHeight(effectiveToolbarH + scrollBarExtent + extraPad);
    }
    m_toolbarArea->setWidget(m_toolbar.get());
    updateToolbarContentWidth();
    QTimer::singleShot(0, this, &TextEditor::updateToolbarContentWidth);

    // Touch/gesture scrolling on the scroll area's viewport (Qt 6)
    m_toolbarArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
    QScroller::grabGesture(m_toolbarArea->viewport(), QScroller::TouchGesture);
    if (QScroller* scroller = QScroller::scroller(m_toolbarArea->viewport())) {
        QScrollerProperties sp = scroller->scrollerProperties();
        sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootWhenScrollable);
        sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    // Strongly lock to horizontal to help prevent tiny vertical drift
    sp.setScrollMetric(QScrollerProperties::AxisLockThreshold, 1.0);
        sp.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.05);
        sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.01);
        sp.setScrollMetric(QScrollerProperties::MaximumVelocity, 1.0);
        scroller->setScrollerProperties(sp);
#ifdef Q_OS_ANDROID
        connect(scroller, &QScroller::stateChanged, this, &TextEditor::updateOverscrollIndicators);
        connect(scroller, &QScroller::scrollerPropertiesChanged, this, &TextEditor::updateOverscrollIndicators);
#endif
    }

    // Monitor scrollbar to update indicators
#ifdef Q_OS_ANDROID
    connect(m_toolbarArea->horizontalScrollBar(), &QScrollBar::valueChanged, this, &TextEditor::updateOverscrollIndicators);
    connect(m_toolbarArea->horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int){ updateOverscrollIndicators(); });
#endif

    // Install event filter on the viewport for touch/focus handling
    m_toolbarArea->viewport()->installEventFilter(this);

    // Set up layout with proper flexbox-like behavior
    // Note: QVBoxLayout parent ownership - when passing 'this', the layout is automatically
    // set and managed by the widget. No need to store in a member variable.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);  // No margins for full expansion
    layout->setSpacing(0);  // No spacing between toolbar and editor
    layout->addWidget(m_toolbarArea.get());
    layout->addWidget(m_editorContainer.get());
    
    // Set stretch factors for flexbox-like behavior
    layout->setStretchFactor(m_toolbarArea.get(), 0);  // Toolbar area doesn't stretch
    layout->setStretchFactor(m_editorContainer.get(), 1);   // Editor container expands to fill available space
    
    // Ensure the text editor widget expands to fill available space
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TextEditor::setupToolbar()
{
    // Create a more responsive toolbar layout that handles overflow better
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    
    // Add font selector with improved touch support for Android as the first item
    m_fontCombo = QuteNote::makeOwned<QFontComboBox>(this);
    m_fontCombo->setWritingSystem(QFontDatabase::Latin);
    Theme theme = ThemeManager::instance()->currentTheme();
    m_fontCombo->setFixedHeight(theme.metrics.touchTarget);
    m_fontCombo->setMaximumWidth(120);  // Increased width for better visibility
    m_fontCombo->setFont(theme.headerFont);
    // Styling for the font combo is provided centrally by ThemeManager.
    
    // Ensure the font combo box is visible and properly integrated
    m_fontCombo->setObjectName("fontComboBox");
    m_fontCombo->setVisible(true);
    m_fontCombo->setEnabled(true);
    QAction* fontAction = m_toolbar->addWidget(m_fontCombo.get());
    
    // Ensure the action is properly configured
    if (fontAction) {
        fontAction->setVisible(true);
    }
    
#ifdef Q_OS_ANDROID
    m_fontCombo->setAttribute(Qt::WA_AcceptTouchEvents, true);
#endif

    // Font size combo with improved touch support for Android as the second item
    m_sizeCombo = QuteNote::makeOwned<QComboBox>(this);
    m_sizeCombo->setEditable(true);
    m_sizeCombo->setInsertPolicy(QComboBox::NoInsert);
    m_sizeCombo->setMinimumContentsLength(3);
    m_sizeCombo->setMaxVisibleItems(12);  // More visible items
    m_sizeCombo->setFixedHeight(theme.metrics.touchTarget);
    m_sizeCombo->setMaximumWidth(80);     // Increased width for better visibility
    m_sizeCombo->setFont(theme.headerFont);
    // Styling for the size combo is provided centrally by ThemeManager.
    
    // Add common font sizes with better spacing for mobile
    const QList<int> standardSizes = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
    for (int size : standardSizes) {
        m_sizeCombo->addItem(QString::number(size));
    }
    m_sizeCombo->setCurrentText("12");
    
    // Ensure the size combo box is visible and properly integrated
    m_sizeCombo->setObjectName("fontSizeComboBox");
    m_sizeCombo->setVisible(true);
    m_sizeCombo->setEnabled(true);
    QAction* sizeAction = m_toolbar->addWidget(m_sizeCombo.get());
    
    // Ensure the action is properly configured
    if (sizeAction) {
        sizeAction->setVisible(true);
    }
    
    // Set sizing directly on the combo box
    m_sizeCombo->setMinimumWidth(80);
#ifdef Q_OS_ANDROID
    m_sizeCombo->setAttribute(Qt::WA_AcceptTouchEvents, true);
#endif
    
    // Also connect editing finished for custom sizes
    connect(m_sizeCombo.get(), &QComboBox::editTextChanged, this, [this](const QString &text) {
        // Only apply when user finishes editing
        if (m_sizeCombo->hasFocus()) {
            QTimer::singleShot(100, this, [this, text]() {
                if (!m_sizeCombo->hasFocus()) {
                    onFontSizeChanged(text);
                }
            });
        }
    });

    // Add separator after font controls
    m_toolbar->addSeparator();
    
    // Add all actions directly to the toolbar for better overflow handling
    // Ensure each button is properly configured for touch events
    QList<QAction*> actions;
    actions << m_boldAction.get() << m_italicAction.get() << m_underlineAction.get();
    
    for (QAction* action : actions) {
        m_toolbar->addAction(action);
        // Ensure the action's associated button is touch-friendly
        QWidget* widget = m_toolbar->widgetForAction(action);
        if (QToolButton* button = qobject_cast<QToolButton*>(widget)) {
            button->setMinimumSize(theme.metrics.touchTarget, theme.metrics.touchTarget);
            button->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
            #ifdef Q_OS_ANDROID
            button->setAttribute(Qt::WA_AcceptTouchEvents, true);
            button->setFocusPolicy(Qt::NoFocus);
#endif
        }
    }
    
    m_toolbar->addSeparator();
    
    // Alignment actions
    actions.clear();
    actions << m_alignLeftAction.get() << m_alignCenterAction.get() 
            << m_alignRightAction.get() << m_alignJustifyAction.get();
    
    for (QAction* action : actions) {
        m_toolbar->addAction(action);
        // Ensure the action's associated button is touch-friendly
        QWidget* widget = m_toolbar->widgetForAction(action);
        if (QToolButton* button = qobject_cast<QToolButton*>(widget)) {
            button->setMinimumSize(theme.metrics.touchTarget, theme.metrics.touchTarget);
            button->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
            #ifdef Q_OS_ANDROID
            button->setAttribute(Qt::WA_AcceptTouchEvents, true);
            button->setFocusPolicy(Qt::NoFocus);
#endif
        }
    }
    
    m_toolbar->addSeparator();
    
    // Color actions
    actions.clear();
    actions << m_textColorAction.get() << m_bgColorAction.get();
    
    for (QAction* action : actions) {
        m_toolbar->addAction(action);
        // Ensure the action's associated button is touch-friendly
        QWidget* widget = m_toolbar->widgetForAction(action);
        if (QToolButton* button = qobject_cast<QToolButton*>(widget)) {
            button->setMinimumSize(theme.metrics.touchTarget, theme.metrics.touchTarget);
            button->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
            #ifdef Q_OS_ANDROID
            button->setAttribute(Qt::WA_AcceptTouchEvents, true);
            button->setFocusPolicy(Qt::NoFocus);
#endif
        }
    }
    
    m_toolbar->addSeparator();
    
    // List actions
    actions.clear();
    actions << m_bulletListAction.get() << m_numberedListAction.get();
    
    for (QAction* action : actions) {
        m_toolbar->addAction(action);
        // Ensure the action's associated button is touch-friendly
        QWidget* widget = m_toolbar->widgetForAction(action);
        if (QToolButton* button = qobject_cast<QToolButton*>(widget)) {
            button->setMinimumSize(theme.metrics.touchTarget, theme.metrics.touchTarget);
            button->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
            #ifdef Q_OS_ANDROID
            button->setAttribute(Qt::WA_AcceptTouchEvents, true);
            button->setFocusPolicy(Qt::NoFocus);
#endif
        }
    }
    
    m_toolbar->addSeparator();
    
    // Insert actions
    actions.clear();
    actions << m_linkAction.get() << m_imageAction.get();
    
    for (QAction* action : actions) {
        m_toolbar->addAction(action);
        // Ensure the action's associated button is touch-friendly
        QWidget* widget = m_toolbar->widgetForAction(action);
        if (QToolButton* button = qobject_cast<QToolButton*>(widget)) {
            button->setMinimumSize(theme.metrics.touchTarget, theme.metrics.touchTarget);
            button->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
            #ifdef Q_OS_ANDROID
            button->setAttribute(Qt::WA_AcceptTouchEvents, true);
            button->setFocusPolicy(Qt::NoFocus);
#endif
        }
    }
    
    // Undo/redo actions are now only in the MainView toolbar; removed from TextEditor toolbar.
    // Add separator before any additional toolbar elements
    m_toolbar->addSeparator();
    
    // Note: Overscroll indicators are now handled as overlay widgets
    // They are no longer added to the toolbar to prevent clipping issues
    
    // Ensure toolbar is properly sized for mobile (fixed height to avoid jiggle)
    {
        const int toolbarVPadding = 8;
        // Use touchTarget for proper height (icon height + room for scrollbar)
        m_toolbar->setFixedHeight(theme.metrics.touchTarget + toolbarVPadding);
    }
    
    // For Android, ensure proper touch handling
#ifdef Q_OS_ANDROID
    m_toolbar->setAttribute(Qt::WA_AcceptTouchEvents, true);
    
    // Ensure combo boxes are properly visible on Android
    if (m_fontCombo) {
        m_fontCombo->setVisible(true);
        m_fontCombo->setEnabled(true);
    }
    if (m_sizeCombo) {
        m_sizeCombo->setVisible(true);
        m_sizeCombo->setEnabled(true);
    }
    // Note: Overscroll indicators are now handled as overlay widgets
    // Their connections are handled in setupUI()
#endif

    // Ensure the toolbar reports its full width so scroll area can expose all actions
    updateToolbarContentWidth();
}

void TextEditor::createToolbarSection(const QString &title, const QList<QAction*> &actions)
{
    // This method is kept for compatibility but we're using a more direct approach
    // in setupToolbar for better overflow handling
    for (QAction* action : actions) {
        m_toolbar->addAction(action);
    }
    m_toolbar->addSeparator();
}

void TextEditor::setupEditor()
{
    // --- Editor Container Setup ---
    // This container now handles the border and padding.
    m_editorContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QVBoxLayout* editorLayout = new QVBoxLayout(m_editorContainer.get());
    // Use 0 margins on the layout, padding is handled by the stylesheet now.
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    editorLayout->addWidget(m_editor.get());

    // --- QTextEdit (m_editor) Setup ---
    // The editor itself now has a clean geometry, no border, no padding.
    m_editor->setAcceptRichText(true);
    UIUtils::makeTouchFriendly(m_editor.get());

    // Configure editor settings
    m_editor->setTabStopDistance(40);
    m_editor->setFrameShape(QFrame::NoFrame); // No frame, border is on the container
    m_editor->setContentsMargins(0, 0, 0, 0); // No margins
    
    // Set default font
    QFont defaultFont = m_editor->font();
    defaultFont.setPointSize(16);  // Larger default font size for better readability
    m_editor->setFont(defaultFont);
    fontChanged(defaultFont);
    
    // Ensure the editor expands to fill available space with no constraints
    m_editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_editor->setMinimumHeight(0);  // Remove minimum height constraint
    m_editor->setMinimumWidth(0);   // Remove minimum width constraint
    // Ensure no maximum size constraints
    m_editor->setMaximumHeight(QWIDGETSIZE_MAX);
    m_editor->setMaximumWidth(QWIDGETSIZE_MAX);
    
    // Ensure the editor properly responds to size changes
    m_editor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    
    // Configure the viewport to expand properly
    if (QWidget* viewport = m_editor->viewport()) {
        viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        viewport->setMinimumHeight(0);  // Remove minimum height constraint
        viewport->setMinimumWidth(0);   // Remove minimum width constraint
        // Ensure no maximum size constraints
        viewport->setMaximumHeight(QWIDGETSIZE_MAX);
        viewport->setMaximumWidth(QWIDGETSIZE_MAX);
    }
    
    // Ensure proper touch handling
#ifdef Q_OS_ANDROID
    m_editor->setAttribute(Qt::WA_AcceptTouchEvents, true);
    m_editor->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
    
    // DO NOT use QScroller on QTextEdit for Android - it interferes with native text selection
    // QTextEdit has built-in touch scrolling that works properly on Android
    // Using QScroller prevents selection handles from appearing
    
    // Enable proper text interaction for selection handles
    m_editor->setTextInteractionFlags(Qt::TextEditorInteraction);
    m_editor->setContextMenuPolicy(Qt::DefaultContextMenu); // Allow native context menu for copy/paste
    
    // Fix InputConnection warnings by properly managing input method
    m_editor->setAttribute(Qt::WA_InputMethodEnabled, true);
    m_editor->setFocusPolicy(Qt::StrongFocus);
    m_editor->setFocusProxy(m_editor.get());
    
    // Ensure the editor can receive focus properly
    m_editor->setFocus(Qt::OtherFocusReason);
    
    // Additional fix for proper resizing
    m_editor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Set focus proxy for the text editor widget itself
    setFocusProxy(m_editor.get());
    
    // Ensure proper focus handling
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    
    // Set initial focus to editor after a short delay
#endif
}

void TextEditor::setupActions()
{
    // Formatting actions with tooltips instead of text
    m_boldAction = QuteNote::makeOwned<QAction>(this);
    m_boldAction->setIcon(QIcon(":/resources/icons/custom/bold.svg"));
    m_boldAction->setToolTip(tr("Bold"));
    m_boldAction->setShortcut(QKeySequence::Bold);
    m_boldAction->setCheckable(true);
    connect(m_boldAction.get(), &QAction::triggered, this, &TextEditor::onBold);

    m_italicAction = QuteNote::makeOwned<QAction>(this);
    m_italicAction->setIcon(QIcon(":/resources/icons/custom/italic.svg"));
    m_italicAction->setToolTip(tr("Italic"));
    m_italicAction->setShortcut(QKeySequence::Italic);
    m_italicAction->setCheckable(true);
    connect(m_italicAction.get(), &QAction::triggered, this, &TextEditor::onItalic);

    m_underlineAction = QuteNote::makeOwned<QAction>(this);
    m_underlineAction->setIcon(QIcon(":/resources/icons/custom/underline.svg"));
    m_underlineAction->setToolTip(tr("Underline"));
    m_underlineAction->setShortcut(QKeySequence::Underline);
    m_underlineAction->setCheckable(true);
    connect(m_underlineAction.get(), &QAction::triggered, this, &TextEditor::onUnderline);

    m_strikeAction = QuteNote::makeOwned<QAction>(this);
    m_strikeAction->setIcon(QIcon(":/resources/icons/custom/strike.svg"));
    m_strikeAction->setToolTip(tr("Strikethrough"));
    m_strikeAction->setCheckable(true);
    connect(m_strikeAction.get(), &QAction::triggered, this, &TextEditor::onStrikethrough);

    m_textColorAction = QuteNote::makeOwned<QAction>(this);
    m_textColorAction->setIcon(QIcon(":/resources/icons/custom/text-color.svg"));
    m_textColorAction->setToolTip(tr("Text Color"));
    connect(m_textColorAction.get(), &QAction::triggered, this, &TextEditor::onTextColor);

    m_bgColorAction = QuteNote::makeOwned<QAction>(tr("Background Color"), this);
    m_bgColorAction->setIcon(QIcon(":/resources/icons/custom/format-painter.svg"));
    m_bgColorAction->setToolTip(tr("Background Color"));
    connect(m_bgColorAction.get(), &QAction::triggered, this, &TextEditor::onBackgroundColor);

    // Alignment actions
    m_alignLeftAction = QuteNote::makeOwned<QAction>("Left", this);
    m_alignLeftAction->setIcon(QIcon(":/resources/icons/custom/align-left.svg"));
    m_alignLeftAction->setCheckable(true);
    connect(m_alignLeftAction.get(), &QAction::triggered, this, &TextEditor::onAlignLeft);

    m_alignCenterAction = QuteNote::makeOwned<QAction>("Center", this);
    m_alignCenterAction->setIcon(QIcon(":/resources/icons/custom/align-center.svg"));
    m_alignCenterAction->setCheckable(true);
    connect(m_alignCenterAction.get(), &QAction::triggered, this, &TextEditor::onAlignCenter);

    m_alignRightAction = QuteNote::makeOwned<QAction>("Right", this);
    m_alignRightAction->setIcon(QIcon(":/resources/icons/custom/align-right.svg"));
    m_alignRightAction->setCheckable(true);
    connect(m_alignRightAction.get(), &QAction::triggered, this, &TextEditor::onAlignRight);

    m_alignJustifyAction = QuteNote::makeOwned<QAction>("Justify", this);
    m_alignJustifyAction->setIcon(QIcon(":/resources/icons/custom/align-justify.svg"));
    m_alignJustifyAction->setCheckable(true);
    connect(m_alignJustifyAction.get(), &QAction::triggered, this, &TextEditor::onAlignJustify);

    // List actions
    m_bulletListAction = QuteNote::makeOwned<QAction>("Bullet List", this);
    m_bulletListAction->setIcon(QIcon(":/resources/icons/custom/list-bullet.svg"));
    m_bulletListAction->setCheckable(true);
    connect(m_bulletListAction.get(), &QAction::triggered, this, &TextEditor::onBulletList);

    m_numberedListAction = QuteNote::makeOwned<QAction>("Numbered List", this);
    m_numberedListAction->setIcon(QIcon(":/resources/icons/custom/list-numbered.svg"));
    m_numberedListAction->setCheckable(true);
    connect(m_numberedListAction.get(), &QAction::triggered, this, &TextEditor::onNumberedList);

    // Insert actions
    m_linkAction = QuteNote::makeOwned<QAction>("Link", this);
    m_linkAction->setIcon(QIcon(":/resources/icons/custom/link.svg"));
    connect(m_linkAction.get(), &QAction::triggered, this, &TextEditor::onInsertLink);

    m_imageAction = QuteNote::makeOwned<QAction>("Image", this);
    m_imageAction->setIcon(QIcon(":/resources/icons/custom/image.svg"));
    connect(m_imageAction.get(), &QAction::triggered, this, &TextEditor::onInsertImage);

    // Undo/Redo actions
    m_undoAction = QuteNote::makeOwned<QAction>("Undo", this);
    m_undoAction->setIcon(QIcon(":/resources/icons/custom/undo.svg"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction.get(), &QAction::triggered, this, &TextEditor::undo);
    m_redoAction = QuteNote::makeOwned<QAction>("Redo", this);
    m_redoAction->setIcon(QIcon(":/resources/icons/custom/redo.svg"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction.get(), &QAction::triggered, this, &TextEditor::redo);
    
    // Create overscroll overlay widgets (positioned independently of toolbar)
    m_overscrollLeftWidget = QuteNote::makeOwned<QToolButton>(this);
    m_overscrollLeftWidget->setIcon(QIcon(":/resources/icons/custom/chevrons-left.svg"));
    // Initial conservative sizing; final size will be applied by applyOverlayButtonTheme()
    m_overscrollLeftWidget->setMinimumSize(20, 40);
    m_overscrollLeftWidget->setIconSize(QSize(24, 24));
    m_overscrollLeftWidget->hide();
    
    m_overscrollRightWidget = QuteNote::makeOwned<QToolButton>(this);
    m_overscrollRightWidget->setIcon(QIcon(":/resources/icons/custom/chevrons-right.svg"));
    // Initial conservative sizing; final size will be applied by applyOverlayButtonTheme()
    m_overscrollRightWidget->setMinimumSize(20, 40);
    m_overscrollRightWidget->setIconSize(QSize(24, 24));
    m_overscrollRightWidget->hide();
    
    // Apply consistent styling to overlay widgets
    QString overlayStyle = 
        "QToolButton {"
        "    border: none;"
        "    background: rgba(0, 0, 0, 0.1);"
        "    padding: 2px;"
        "    margin: 1px;"
        "    border-radius: 4px;"
        "}"
        "QToolButton:disabled {"
        "    opacity: 0.3;"
        "}"
        "QToolButton:hover {"
        "    background: rgba(0, 0, 0, 0.2);"
        "}"
        "QToolButton:pressed {"
        "    background: rgba(0, 0, 0, 0.3);"
        "}";
    
    m_overscrollLeftWidget->setStyleSheet(overlayStyle);
    m_overscrollRightWidget->setStyleSheet(overlayStyle);

    // Apply theme-based sizing and colors
    applyOverlayButtonTheme();
    
#ifdef Q_OS_ANDROID
    m_overscrollLeftWidget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    m_overscrollLeftWidget->setFocusPolicy(Qt::NoFocus);
    m_overscrollLeftWidget->setCursor(Qt::PointingHandCursor);
    // Ensure the widget can handle mouse events (which are generated from touch events)
    m_overscrollLeftWidget->setAttribute(Qt::WA_StaticContents, false);
    
    m_overscrollRightWidget->setAttribute(Qt::WA_AcceptTouchEvents, true);
    m_overscrollRightWidget->setFocusPolicy(Qt::NoFocus);
    m_overscrollRightWidget->setCursor(Qt::PointingHandCursor);
    // Ensure the widget can handle mouse events (which are generated from touch events)
    m_overscrollRightWidget->setAttribute(Qt::WA_StaticContents, false);
#endif

    // Re-apply overlay theme when the application theme changes
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](const Theme&){
        updateToolbarTheme();
        applyOverlayButtonTheme();
        updateOverscrollIndicators();
    });
}

void TextEditor::applyOverlayButtonTheme()
{
    if (!m_overscrollLeftWidget || !m_overscrollRightWidget) return;
    Theme theme = ThemeManager::instance()->currentTheme();

    // Slightly smaller than touchTarget to avoid overlap; keep at least 36 tall
    const int height = qMax(36, theme.metrics.touchTarget - 8);
    const int width = qMax(18, height / 2); // make overscroll half as wide as tall
    const int iconSz = qMax(20, theme.metrics.iconSize);

    auto setBtn = [&](QToolButton* btn){
        if (!btn) return;
        // Lock the overlay button to the exact desired size so it doesn't get stretched by layouts
        btn->setFixedSize(width, height);
        btn->setIconSize(QSize(iconSz, iconSz));
        btn->setCursor(Qt::PointingHandCursor);

        const QColor base = theme.colors.text; // neutral overlay based on text color
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

    // Update after theme apply
    if (layout()) layout()->activate();

    // Toolbar button sizes depend on icon metrics, so refresh width bounds too
    updateToolbarContentWidth();
}

void TextEditor::updateToolbarTheme()
{
    if (!m_toolbar) return;
    
    Theme theme = ThemeManager::instance()->currentTheme();
    
    // Update all toolbar buttons
    const QList<QAction*> actions = m_toolbar->actions();
    for (QAction* action : actions) {
        QWidget* widget = m_toolbar->widgetForAction(action);
        if (QToolButton* button = qobject_cast<QToolButton*>(widget)) {
            button->setFixedSize(theme.metrics.touchTarget, theme.metrics.touchTarget);
            button->setIconSize(QSize(theme.metrics.iconSize, theme.metrics.iconSize));
        }
    }
    
    // Update comboboxes
    if (m_fontCombo) {
        m_fontCombo->setFixedHeight(theme.metrics.touchTarget);
        m_fontCombo->setFont(theme.headerFont);
        // Set explicit font size in stylesheet to ensure scaling
        m_fontCombo->setStyleSheet(QString("QComboBox { font-size: %1pt; }").arg(theme.headerFont.pointSize()));
    }
    if (m_sizeCombo) {
        m_sizeCombo->setFixedHeight(theme.metrics.touchTarget);
        m_sizeCombo->setFont(theme.headerFont);
        // Set explicit font size in stylesheet to ensure scaling
        m_sizeCombo->setStyleSheet(QString("QComboBox { font-size: %1pt; }").arg(theme.headerFont.pointSize()));
    }
    
    // Update toolbar height
    const int toolbarVPadding = 8;
    m_toolbar->setFixedHeight(theme.metrics.touchTarget + toolbarVPadding);
    
    // Update toolbar area height to match (toolbar + scrollbar)
    if (m_toolbarArea) {
        const int scrollBarExtent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, m_toolbarArea.get());
        const int extraPad = 2;
        int effectiveToolbarH = m_toolbar->height();
        m_toolbarArea->setFixedHeight(effectiveToolbarH + scrollBarExtent + extraPad);
    }
    
    // Refresh toolbar layout
    updateToolbarContentWidth();
}

void TextEditor::updateToolbarContentWidth()
{
    if (!m_toolbar) {
        return;
    }

    if (QLayout *toolLayout = m_toolbar->layout()) {
        toolLayout->invalidate();
        toolLayout->activate();
    }

    const QSize hint = m_toolbar->sizeHint();
    if (hint.width() <= 0) {
        return;
    }

    const bool sizeChanged = (m_toolbar->minimumWidth() != hint.width()) ||
                             (m_toolbar->maximumWidth() != hint.width());

    m_toolbar->setMinimumWidth(hint.width());
    m_toolbar->setMaximumWidth(hint.width());

    if (sizeChanged) {
        m_toolbar->updateGeometry();
        if (m_toolbarArea) {
            if (QWidget *areaWidget = m_toolbarArea->widget()) {
                areaWidget->updateGeometry();
            }
            if (QScrollBar *bar = m_toolbarArea->horizontalScrollBar()) {
                const int viewportWidth = m_toolbarArea->viewport() ? m_toolbarArea->viewport()->width() : 0;
                bar->setRange(0, qMax(0, hint.width() - viewportWidth));
            }
        }
    }
}

void TextEditor::setupMenus()
{
    // Set alignment group
    QActionGroup *alignGroup = new QActionGroup(this);
    alignGroup->addAction(m_alignLeftAction.get());
    alignGroup->addAction(m_alignCenterAction.get());
    alignGroup->addAction(m_alignRightAction.get());
    alignGroup->addAction(m_alignJustifyAction.get());
    m_alignLeftAction->setChecked(true);
}

void TextEditor::setContent(const QString &content)
{
    if (!m_editor) return;
    m_editor->setHtml(content);
    m_modified = false;
    emit modificationChanged(false);
}

QString TextEditor::getContent() const
{
    if (!m_editor) return QString();
    return m_editor->toHtml();
}

void TextEditor::setFilePath(const QString &filePath)
{
    m_filePath = filePath;
    emit filePathChanged(filePath);
}

QString TextEditor::filePath() const
{
    return m_filePath;
}

void TextEditor::setDefaultSaveDirectory(const QString &directory)
{
    m_defaultSaveDirectory = directory;
}

QString TextEditor::defaultSaveDirectory() const
{
    return m_defaultSaveDirectory;
}

bool TextEditor::isModified() const
{
    return m_modified;
}

void TextEditor::setModified(bool modified)
{
    m_modified = modified;
    emit modificationChanged(modified);
}

void TextEditor::onTextChanged()
{
#ifdef Q_OS_ANDROID
    // Set the changing text flag
    m_changingText = true;
#endif
    
    if (!m_modified) {
        m_modified = true;
        emit modificationChanged(true);
    }
    emit contentChanged();
    
#ifdef Q_OS_ANDROID
    // Reset the changing text flag after a short delay
    QTimer::singleShot(50, this, [this]() {
        m_changingText = false;
    });
#endif
}

void TextEditor::onBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(m_boldAction->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditor::onItalic()
{
    QTextCharFormat fmt;
    bool wantItalic = m_italicAction->isChecked();

    // Try to map to an explicit italic font family if one exists (e.g. a
    // separate 'Nunito Sans Italic' family). If not found, fall back to
    // setting the fontItalic flag which uses font synthesis if necessary.
    QString baseFamily = QApplication::font().family();
    QString chosenItalicFamily;
    if (wantItalic) {
        // Look for a family name that indicates an italic variant.
        const QString needle = "Italic";
    const auto allFamilies = QFontDatabase::families();
        for (const QString &family : allFamilies) {
            // Family names for variants sometimes contain the base family name
            // and the word "Italic" (or end with "Italic"). Match conservatively.
            if ((family.contains(baseFamily, Qt::CaseInsensitive) && family.contains(needle, Qt::CaseInsensitive))
                || family.endsWith(needle, Qt::CaseInsensitive)) {
                chosenItalicFamily = family;
                break;
            }
        }
    }

    if (!chosenItalicFamily.isEmpty()) {
        // Use the italic family explicitly and avoid asking QTextEdit to synthesize italics
    fmt.setFontFamilies(QStringList() << chosenItalicFamily);
        fmt.setFontItalic(false);
    } else {
        fmt.setFontItalic(wantItalic);
    }

    mergeFormatOnWordOrSelection(fmt);
}

void TextEditor::onUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(m_underlineAction->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditor::onStrikethrough()
{
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(m_strikeAction->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEditor::onTextColor()
{
    // Ensure we're on the main thread
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "onTextColor", Qt::QueuedConnection);
        return;
    }

    if (!m_editor) return;

    try {
        QColor currentColor = m_editor->textColor();
        QColor color = ColorPicker::getColor(currentColor, this);
        
        if (color.isValid()) {
            QTextCharFormat fmt;
            fmt.setForeground(color);
            mergeFormatOnWordOrSelection(fmt);
        }
    } catch (const std::exception &e) {
        qWarning() << "Error in onTextColor:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error in onTextColor";
    }
}

void TextEditor::onFontChanged(const QFont &font)
{
    // Ensure we're on the main thread
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "onFontChanged", Qt::QueuedConnection, Q_ARG(QFont, font));
        return;
    }

    if (!m_editor) return;
    
    try {
        QTextCharFormat fmt;
        fmt.setFontFamilies(QStringList() << font.family());
        mergeFormatOnWordOrSelection(fmt);
        
        // Update the font combo to reflect the current font
        if (m_fontCombo) {
            m_fontCombo->blockSignals(true);
#ifdef Q_OS_ANDROID
            // On Android, ensure the combo box is visible and properly updated
            m_fontCombo->setVisible(true);
            m_fontCombo->setEnabled(true);
            m_fontCombo->update();
            // Force layout update to ensure proper visibility
            if (m_fontCombo->parentWidget()) {
                m_fontCombo->parentWidget()->update();
            }
#endif
            m_fontCombo->setCurrentFont(font);
            m_fontCombo->blockSignals(false);
        }
    } catch (const std::exception &e) {
        qWarning() << "Error in onFontChanged:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error in onFontChanged";
    }
}

void TextEditor::onFontSizeChanged(const QString &size)
{
    // Ensure we're on the main thread
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "onFontSizeChanged", Qt::QueuedConnection, Q_ARG(QString, size));
        return;
    }

    if (!m_editor) return;
    
    bool ok;
    int fontSize = size.toInt(&ok);
    if (!ok || fontSize <= 0) return;
    
    try {
        QTextCharFormat fmt;
        fmt.setFontPointSize(fontSize);
        mergeFormatOnWordOrSelection(fmt);
        
        // Update the size combo to reflect the current size
        if (m_sizeCombo) {
            m_sizeCombo->blockSignals(true);
#ifdef Q_OS_ANDROID
            // On Android, ensure the combo box is visible and properly updated
            m_sizeCombo->setVisible(true);
            m_sizeCombo->setEnabled(true);
            m_sizeCombo->update();
            // Force layout update to ensure proper visibility
            if (m_sizeCombo->parentWidget()) {
                m_sizeCombo->parentWidget()->update();
            }
#endif
            // Ensure the size is in the list
            if (m_sizeCombo->findText(size) == -1) {
                m_sizeCombo->addItem(size);
            }
            m_sizeCombo->setEditText(size);
            m_sizeCombo->blockSignals(false);
        }
    } catch (const std::exception &e) {
        qWarning() << "Error in onFontSizeChanged:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error in onFontSizeChanged";
    }
}

void TextEditor::onBackgroundColor()
{
    // Ensure we're on the main thread
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "onBackgroundColor", Qt::QueuedConnection);
        return;
    }

    if (!m_editor) return;

    try {
        QColor currentColor = m_editor->textBackgroundColor();
        QColor color = ColorPicker::getColor(currentColor, this);
        
        if (color.isValid()) {
            QTextCharFormat fmt;
            fmt.setBackground(color);
            mergeFormatOnWordOrSelection(fmt);
        }
    } catch (const std::exception &e) {
        qWarning() << "Error in onBackgroundColor:" << e.what();
    } catch (...) {
        qWarning() << "Unknown error in onBackgroundColor";
    }
}

void TextEditor::onAlignLeft()
{
    if (!m_editor) return;
    m_editor->setAlignment(Qt::AlignLeft);
}

void TextEditor::onAlignCenter()
{
    if (!m_editor) return;
    m_editor->setAlignment(Qt::AlignCenter);
}

void TextEditor::onAlignRight()
{
    if (!m_editor) return;
    m_editor->setAlignment(Qt::AlignRight);
}

void TextEditor::onAlignJustify()
{
    if (!m_editor) return;
    m_editor->setAlignment(Qt::AlignJustify);
}

void TextEditor::onBulletList()
{
    applyFormat(QTextListFormat::ListDisc);
}

void TextEditor::onNumberedList()
{
    applyFormat(QTextListFormat::ListDecimal);
}

void TextEditor::onInsertLink()
{
    if (!m_editor) return;
    
    bool ok;
    QString url = QInputDialog::getText(this, "Insert Link",
                                       "URL:", QLineEdit::Normal,
                                       "", &ok);
    if (ok && !url.isEmpty()) {
        QTextCursor cursor = m_editor->textCursor();
        QTextCharFormat format;
        format.setAnchor(true);
        format.setAnchorHref(url);
        format.setForeground(Qt::blue);
        format.setFontUnderline(true);
        cursor.mergeCharFormat(format);
        cursor.insertText(url);
    }
}

void TextEditor::onInsertImage()
{
    if (!m_editor) return;
    
    QString fileName = QFileDialog::getOpenFileName(this,
        "Insert Image", "", "Images (*.png *.jpg *.jpeg *.gif *.bmp)");
    if (!fileName.isEmpty()) {
        QTextImageFormat imageFormat;
        imageFormat.setName(fileName);
        m_editor->textCursor().insertImage(imageFormat);
    }
}


void TextEditor::onCursorPositionChanged()
{
    // Safety check - ensure editor is initialized
    if (!m_editor) {
        return;
    }
    
    QTextCharFormat fmt = m_editor->currentCharFormat();
    fontChanged(fmt.font());

    // Update action states for text formatting
    if (m_boldAction) m_boldAction->setChecked(fmt.fontWeight() == QFont::Bold);
    if (m_italicAction) m_italicAction->setChecked(fmt.fontItalic());
    if (m_underlineAction) m_underlineAction->setChecked(fmt.fontUnderline());
    
    // Update list button states based on current list
    QTextCursor cursor = m_editor->textCursor();
    QTextList* currentList = cursor.currentList();
    
    if (currentList) {
        QTextListFormat listFormat = currentList->format();
        QTextListFormat::Style listStyle = listFormat.style();
        
        if (m_bulletListAction) {
            m_bulletListAction->setChecked(listStyle == QTextListFormat::ListDisc);
        }
        if (m_numberedListAction) {
            m_numberedListAction->setChecked(listStyle == QTextListFormat::ListDecimal);
        }
    } else {
        // Not in a list - uncheck both list buttons
        if (m_bulletListAction) m_bulletListAction->setChecked(false);
        if (m_numberedListAction) m_numberedListAction->setChecked(false);
    }
    
#ifdef Q_OS_ANDROID
    // Refresh toolbar on Android to ensure proper visibility
    refreshToolbar();
    // Update overscroll indicators
    updateOverscrollIndicators();
    
    // Additional refresh for combo boxes
    if (m_fontCombo) {
        m_fontCombo->setVisible(true);
        m_fontCombo->setEnabled(true);
    }
    if (m_sizeCombo) {
        m_sizeCombo->setVisible(true);
        m_sizeCombo->setEnabled(true);
    }
#endif
}

void TextEditor::undo()
{
    if (!m_editor) return;
    m_editor->undo();
}

void TextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    if (!m_editor) return;
    QTextCursor cursor = m_editor->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    m_editor->mergeCurrentCharFormat(format);
}

void TextEditor::fontChanged(const QFont &f)
{
    // Safety check for Android - ensure combo boxes are initialized
    if (!m_fontCombo || !m_sizeCombo) {
        return;
    }
    
    // Block signals to prevent recursive updates
    m_fontCombo->blockSignals(true);
    m_sizeCombo->blockSignals(true);
    
    // For Android, use setCurrentFont instead of index-based selection for better reliability
    m_fontCombo->setCurrentFont(f);
    
    // Ensure the font combo box is visible and properly updated
#ifdef Q_OS_ANDROID
    m_fontCombo->setVisible(true);
    m_fontCombo->setEnabled(true);
    // Force update for Android
    m_fontCombo->update();
    // Update parent layout if needed
    if (m_fontCombo->parentWidget()) {
        m_fontCombo->parentWidget()->update();
    }
#endif
    
    // Set the size text directly, handling the case where pointSize() returns -1
    int pointSize = f.pointSize();
    QString sizeText = (pointSize > 0) ? QString::number(pointSize) : "12"; // Default to 12 if invalid
    if (pointSize > 0 && m_sizeCombo->findText(sizeText) == -1) {
        // If the size isn't in the list, add it
        m_sizeCombo->addItem(sizeText);
    }
    m_sizeCombo->setEditText(sizeText);
    
    // Ensure the size combo box is visible and properly updated
#ifdef Q_OS_ANDROID
    m_sizeCombo->setVisible(true);
    m_sizeCombo->setEnabled(true);
    // Force update for Android
    m_sizeCombo->update();
    // Update parent layout if needed
    if (m_sizeCombo->parentWidget()) {
        m_sizeCombo->parentWidget()->update();
    }
#endif
    
    m_fontCombo->blockSignals(false);
    m_sizeCombo->blockSignals(false);
    
#ifdef Q_OS_ANDROID
    // Additional refresh for Android
    QTimer::singleShot(10, this, [this]() {
        if (m_fontCombo) m_fontCombo->update();
        if (m_sizeCombo) m_sizeCombo->update();
    });
#endif
}

void TextEditor::applyFormat(QTextListFormat::Style style)
{
    if (!m_editor) return;
    
    QTextCursor cursor = m_editor->textCursor();
    cursor.beginEditBlock();
    
    QTextList* currentList = cursor.currentList();
    
    if (currentList) {
        // If already in a list, check if it's the same style
        QTextListFormat currentFormat = currentList->format();
        
        if (currentFormat.style() == style) {
            // Same style - remove from list and reset to plain paragraph formatting
            currentList->remove(cursor.block());
            
            // Create a clean block format with no indentation
            QTextBlockFormat plainFormat;
            plainFormat.setIndent(0);
            cursor.setBlockFormat(plainFormat);
        } else {
            // Different style - change the list style while preserving indent
            QTextListFormat newFormat;
            newFormat.setStyle(style);
            newFormat.setIndent(currentFormat.indent());
            currentList->setFormat(newFormat);
        }
    } else {
        // Not in a list - create a new list with proper indentation
        QTextListFormat listFormat;
        listFormat.setStyle(style);
        
        // Get the current block format to check indentation
        QTextBlockFormat blockFormat = cursor.blockFormat();
        
        // Preserve existing block indentation or start at indent level 1
        int currentIndent = blockFormat.indent();
        if (currentIndent > 0) {
            listFormat.setIndent(currentIndent);
            blockFormat.setIndent(0);
            cursor.setBlockFormat(blockFormat);
        } else {
            listFormat.setIndent(1);
        }
        
        // For numbered lists, check if there's a previous list to continue from
        if (style == QTextListFormat::ListDecimal) {
            // Look for the previous block to check if it's a numbered list
            QTextBlock prevBlock = cursor.block().previous();
            while (prevBlock.isValid()) {
                QTextList* prevList = prevBlock.textList();
                if (prevList && prevList->format().style() == QTextListFormat::ListDecimal) {
                    // Found a previous numbered list - continue numbering from it
                    int prevIndent = prevList->format().indent();
                    if (prevIndent == listFormat.indent()) {
                        // Same indent level - merge with the previous list
                        prevList->add(cursor.block());
                        cursor.endEditBlock();
                        return;
                    }
                    break;
                }
                // Skip non-list blocks (like empty lines)
                if (prevBlock.text().trimmed().isEmpty() && !prevBlock.textList()) {
                    prevBlock = prevBlock.previous();
                    continue;
                }
                break;
            }
        }
        
        cursor.createList(listFormat);
    }
    
    cursor.endEditBlock();
}

#ifdef Q_OS_ANDROID
void TextEditor::refreshToolbar()
{
    // Ensure all toolbar elements are visible and properly configured on Android
    if (m_toolbar) {
        m_toolbar->setVisible(true);
        m_toolbar->setEnabled(true);
        
        // Refresh the toolbar layout
        m_toolbar->update();
        m_toolbar->repaint();
        
        // Ensure proper sizing
        m_toolbar->adjustSize();
    }
    
    // Ensure font combo box is visible and properly configured
    if (m_fontCombo) {
        m_fontCombo->setVisible(true);
        m_fontCombo->setEnabled(true);
        m_fontCombo->update();
        
        // Force refresh of font combo box content
        QFont currentFont = m_fontCombo->currentFont();
        m_fontCombo->blockSignals(true);
        m_fontCombo->setCurrentFont(currentFont);
        m_fontCombo->blockSignals(false);
    }
    
    // Ensure size combo box is visible and properly configured
    if (m_sizeCombo) {
        m_sizeCombo->setVisible(true);
        m_sizeCombo->setEnabled(true);
        m_sizeCombo->update();
        
        // Force refresh of size combo box content
        QString currentSize = m_sizeCombo->currentText();
        m_sizeCombo->blockSignals(true);
        m_sizeCombo->setEditText(currentSize);
        m_sizeCombo->blockSignals(false);
    }
    
    // Update the layout
    if (layout()) {
        layout()->update();
        layout()->activate();
    }
    
    // Update overscroll indicators
    updateOverscrollIndicators();
    
    updateToolbarContentWidth();

    // Additional refresh for Android after a short delay
    QTimer::singleShot(50, this, [this]() {
        if (m_toolbar) m_toolbar->update();
        if (m_fontCombo) m_fontCombo->update();
        if (m_sizeCombo) m_sizeCombo->update();
        updateToolbarContentWidth();
        updateOverscrollIndicators();
    });
}
#endif

void TextEditor::newDocument()
{
    if (!m_editor) return;
    m_editor->clear();
    m_filePath.clear();
    m_modified = false;
    emit modificationChanged(false);
}

void TextEditor::openDocument()
{
    if (!m_editor) return;
    QString fileName = QFileDialog::getOpenFileName(this,
        "Open Document", m_filePath, "HTML files (*.html);;Text files (*.txt);;All files (*.*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            m_editor->setHtml(file.readAll());
            m_filePath = fileName;
            m_modified = false;
            emit modificationChanged(false);
        }
    }
}

void TextEditor::saveDocument()
{
    if (!m_editor) return;
    if (m_filePath.isEmpty()) {
        saveDocumentAs();
    } else {
        QFile file(m_filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_editor->toHtml().toUtf8());
            file.close();
            m_modified = false;
            emit modificationChanged(false);
            emit fileSaved(m_filePath);
        }
    }
}

bool TextEditor::saveDocumentAs()
{
    if (!m_editor) return false;
    
    // Use default save directory if set, otherwise use Documents directory
    QString defaultDir = m_defaultSaveDirectory;
    if (defaultDir.isEmpty()) {
        defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    // Ensure the default directory exists
    QDir().mkpath(defaultDir);
    
    QString fileName = QFileDialog::getSaveFileName(this, "Save Document As",
                                                   defaultDir,
                                                   "HTML Files (*.html);;Text Files (*.txt);;All Files (*.*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(m_editor->toHtml().toUtf8());
            file.close();
            m_filePath = fileName;
            m_modified = false;
            emit modificationChanged(false);
            emit fileSaved(m_filePath);
            return true;
        }
    }
    return false;
}

void TextEditor::cut()
{
    if (!m_editor) return;
    m_editor->cut();
}

void TextEditor::copy()
{
    if (!m_editor) return;
    m_editor->copy();
}

void TextEditor::paste()
{
    if (!m_editor) return;
    m_editor->paste();
}

void TextEditor::redo()
{
    if (!m_editor) return;
    m_editor->redo();
}

void TextEditor::updateOverscrollIndicators()
{
#ifdef Q_OS_ANDROID
    if (!m_toolbarArea || !m_toolbar || !m_overscrollLeftWidget || !m_overscrollRightWidget) return;

    QScrollBar* hbar = m_toolbarArea->horizontalScrollBar();
    if (!hbar) return;

    // Determine overflow and scrollability using the scroll bar
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
        m_overscrollLeftWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
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
        m_overscrollRightWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    } else {
        m_overscrollRightWidget->hide();
    }

    // Ensure indicators refresh after layout settles
    if (hasOverflow) {
        QTimer::singleShot(100, this, [this]() { updateOverscrollIndicators(); });
    }
#endif
}

#ifdef Q_OS_ANDROID
void TextEditor::scrollToolbarLeft()
{
    if (!m_toolbarArea) return;

    if (QScrollBar *bar = m_toolbarArea->horizontalScrollBar()) {
        const int viewportWidth = m_toolbarArea->viewport() ? m_toolbarArea->viewport()->width() : 0;
        const int step = qMax(60, viewportWidth * 6 / 10); // move roughly 60% of visible width
        bar->setValue(qMax(bar->minimum(), bar->value() - step));
        QTimer::singleShot(10, this, [this]() { updateOverscrollIndicators(); });
    }
}

void TextEditor::scrollToolbarRight()
{
    if (!m_toolbarArea) return;

    if (QScrollBar *bar = m_toolbarArea->horizontalScrollBar()) {
        const int viewportWidth = m_toolbarArea->viewport() ? m_toolbarArea->viewport()->width() : 0;
        const int step = qMax(60, viewportWidth * 6 / 10);
        bar->setValue(qMin(bar->maximum(), bar->value() + step));
        QTimer::singleShot(10, this, [this]() { updateOverscrollIndicators(); });
    }
}
#endif

#ifdef Q_OS_ANDROID
void TextEditor::updateEditorGeometry()
{
    if (m_editor) {
        m_editor->updateGeometry();
        if (m_editor->layout()) {
            m_editor->layout()->activate();
        }
    }
}
#endif
