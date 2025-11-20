#include "filebrowsertouchhandler.h"
#include "filebrowser.h"
#include "touchinteraction.h"
#include <QApplication>
#include <QTreeWidget>
#include <QScrollBar>
#include <QtMath>
#include <QElapsedTimer>
#include <QMimeData>
#include <QDrag>
#include <QPixmap>
#include <QPainter>

FileBrowserTouchHandler::FileBrowserTouchHandler(FileBrowser* fileBrowser)
    : TouchInteractionHandler(fileBrowser)
    , m_fileBrowser(fileBrowser)
    , m_scroller(nullptr)
    , m_touchInteraction(new TouchInteraction(this))
    , m_lastTouchedItem(nullptr)
{
    if (!m_fileBrowser) return;
    
    enableGestureHandling(m_fileBrowser->treeWidget());
    setupScrolling();
    
    // Configure touch interaction physics
    m_touchInteraction->setBouncePreset(TouchInteraction::Normal);
    
    // Connect touch interaction signals
    connect(m_touchInteraction, &TouchInteraction::overscrollAmountChanged,
            this, &FileBrowserTouchHandler::overscrollAmountChanged);

    // Long-press timer for initiating item drags explicitly
    m_longPressTimer = new QTimer(this);
    m_longPressTimer->setSingleShot(true);
    connect(m_longPressTimer, &QTimer::timeout, this, [this]() {
        if (!m_fileBrowser || !m_fileBrowser->treeWidget()) return;
        QTreeWidget* tw = m_fileBrowser->treeWidget();
        if (!m_lastTouchedItem) return;

        // Ensure touch hasn't moved too far and is still within the tree widget bounds
        QPoint globalPos = tw->mapToGlobal(m_lastTouchPos);
        QPoint posInTree = tw->mapFromGlobal(globalPos);
        if (!tw->rect().contains(posInTree)) {
            return; // finger moved outside before long-press triggered
        }
        int distance = (m_lastTouchPos - m_touchStartPos).manhattanLength();
        if (distance > DRAG_THRESHOLD) {
            return; // treat as scroll/pan, not drag
        }

        m_longPressTriggered = true;
        m_isItemDrag = true;

        // Initiate the QTreeWidget's internal drag operation
        tw->setCurrentItem(m_lastTouchedItem); // Ensure the item is selected for drag
        m_fileBrowser->treeWidget()->initiateDrag(Qt::MoveAction, m_touchStartPos);
    });
}

FileBrowserTouchHandler::~FileBrowserTouchHandler()
{
    if (m_fileBrowser && m_fileBrowser->treeWidget()) {
        disableGestureHandling(m_fileBrowser->treeWidget());
        QScroller::ungrabGesture(m_fileBrowser->treeWidget()->viewport());
    }
}

void FileBrowserTouchHandler::setupScrolling()
{
    auto treeWidget = m_fileBrowser->treeWidget();
    if (!treeWidget || !treeWidget->viewport()) return;

    m_scroller = QScroller::scroller(treeWidget->viewport());
    QScrollerProperties props = m_scroller->scrollerProperties();
    
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, 
                         QScrollerProperties::OvershootWhenScrollable);
    props.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.5);
    props.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.3);
    
    m_scroller->setScrollerProperties(props);
    QScroller::grabGesture(treeWidget->viewport(), QScroller::TouchGesture);
    
    updateScrollLimits();
}

void FileBrowserTouchHandler::updateScrollLimits()
{
    auto treeWidget = m_fileBrowser->treeWidget();
    if (!treeWidget || !treeWidget->viewport()) return;

    const int viewportHeight = treeWidget->viewport()->height();
    const int contentHeight = treeWidget->sizeHint().height();
    
    const int min = 0;
    const int max = qMax(0, contentHeight - viewportHeight);
    
    m_touchInteraction->setScrollLimits(min, max);
}

QTreeWidgetItem* FileBrowserTouchHandler::itemAtPoint(const QPoint& point) const
{
    auto treeWidget = m_fileBrowser->treeWidget();
    if (!treeWidget) return nullptr;
    
    return treeWidget->itemAt(point);
}

void FileBrowserTouchHandler::handleItemTap(QTreeWidgetItem* item)
{
    if (!item) return;
    
    emit itemTapped(item);
    if (item->childCount() > 0) {
        emit itemExpansionRequested(item);
    }
}

void FileBrowserTouchHandler::handlePinchGesture(QPinchGesture* gesture)
{
    if (!gesture || !m_fileBrowser) return;

    // Handle pinch to zoom folder view (if implemented)
}

void FileBrowserTouchHandler::handleSwipeGesture(QSwipeGesture* gesture)
{
    if (!gesture || !m_fileBrowser) return;

    if (gesture->state() == Qt::GestureFinished) {
        if (gesture->horizontalDirection() == QSwipeGesture::Left) {
            // Navigate forward in folder history
            m_fileBrowser->navigateForward();
        } else if (gesture->horizontalDirection() == QSwipeGesture::Right) {
            // Navigate backward in folder history
            m_fileBrowser->navigateBack();
        }
    }
}

void FileBrowserTouchHandler::handlePanGesture(QPanGesture* gesture)
{
    // Pan gesture handling is managed by QScroller
}

bool FileBrowserTouchHandler::handleTouchBegin(QTouchEvent* event)
{
    // Check if touch is on a UI element that should handle the event itself
    if (!event->points().isEmpty()) {
        QPoint touchPos = event->points().first().position().toPoint();
        QWidget* widgetAtPoint = qApp->widgetAt(touchPos);
        
        // If the widget at the touch point is a button or other interactive element,
        // let it handle the event instead of processing it as a file browser interaction
        if (widgetAtPoint && (qobject_cast<QAbstractButton*>(widgetAtPoint) || 
                             widgetAtPoint->property("interactiveElement").toBool())) {
            return false; // Let the widget handle the event
        }
    }
    
    bool handled = TouchInteractionHandler::handleTouchBegin(event);
    if (handled && !event->points().isEmpty()) {
        m_touchStartPos = event->points().first().position().toPoint();
        m_lastTouchPos = m_touchStartPos;
        m_lastTouchedItem = itemAtPoint(m_touchStartPos);
        m_isItemDrag = false;
        m_longPressTriggered = false;
        if (m_lastTouchedItem) {
            m_longPressTimer->start(LONG_PRESS_TIMEOUT);
        }
    }
    return handled;
}

bool FileBrowserTouchHandler::handleTouchUpdate(QTouchEvent* event)
{
    bool handled = TouchInteractionHandler::handleTouchUpdate(event);
    if (handled && !event->points().isEmpty() && m_lastTouchedItem) {
        QPoint currentPos = event->points().first().position().toPoint();
        m_lastTouchPos = currentPos;
        int distance = (currentPos - m_touchStartPos).manhattanLength();
        
        // If user moves finger significantly before long-press, cancel the drag initiation
        if (!m_longPressTriggered && distance > DRAG_THRESHOLD) {
            if (m_longPressTimer->isActive()) m_longPressTimer->stop();
        }

        // If finger goes outside the tree widget while waiting, cancel long-press
        if (!m_longPressTriggered) {
            if (auto *tw = m_fileBrowser->treeWidget()) {
                QPoint posInTree = tw->mapFromGlobal(event->points().first().globalPosition().toPoint());
                if (!tw->rect().contains(posInTree)) {
                    if (m_longPressTimer->isActive()) m_longPressTimer->stop();
                    // Also clear the target item to avoid accidental start
                    m_lastTouchedItem = nullptr;
                }
            }
        }
        
        // Get the current scroll position to calculate overscroll
        auto treeWidget = m_fileBrowser->treeWidget();
        if (treeWidget) {
            QScrollBar *vScrollBar = treeWidget->verticalScrollBar();
            if (vScrollBar) {
                int currentValue = vScrollBar->value();
                int minValue = vScrollBar->minimum();
                int maxValue = vScrollBar->maximum();
                
                qreal overscroll = 0.0;
                if (currentValue < minValue) {
                    overscroll = minValue - currentValue;
                } else if (currentValue > maxValue) {
                    overscroll = currentValue - maxValue;
                }
                
                m_touchInteraction->setOverscrollAmount(overscroll);
            }
        }
    }
    return handled;
}

bool FileBrowserTouchHandler::handleTouchEnd(QTouchEvent* event)
{
    bool handled = TouchInteractionHandler::handleTouchEnd(event);
    if (m_longPressTimer && m_longPressTimer->isActive()) {
        m_longPressTimer->stop();
    }
    
    if (handled && m_lastTouchedItem && !m_longPressTriggered) {
        static QElapsedTimer tapTimer;
        if (tapTimer.elapsed() < TAP_TIMEOUT) {
            handleItemTap(m_lastTouchedItem);
        }
        tapTimer.start();
    }
    
    m_lastTouchedItem = nullptr;
    m_isItemDrag = false;
    m_longPressTriggered = false;
    
    return handled;
}