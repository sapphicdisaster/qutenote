#include "texteditortouchhandler.h"
#include "texteditor.h"
#include "touchinteraction.h"
#include <QScrollBar>
#include <QtMath>

TextEditorTouchHandler::TextEditorTouchHandler(TextEditor* textEditor)
    : TouchInteractionHandler(textEditor)
    , m_textEditor(textEditor)
    , m_scroller(nullptr)
    , m_touchInteraction(new TouchInteraction(m_textEditor))
{
    if (!m_textEditor) return;
    
    enableGestureHandling(m_textEditor);
    setupScrolling();
    
    // Configure touch interaction physics
    m_touchInteraction->setBouncePreset(TouchInteraction::Normal);
    connect(m_touchInteraction, &TouchInteraction::bounceScaleChanged,
            m_textEditor, &TextEditor::setZoomFactor);
    
    // Fix Android cursor positioning
#ifdef Q_OS_ANDROID
    // Ensure the text editor viewport accepts touch events
    if (m_textEditor->viewport()) {
        m_textEditor->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, true);
        m_textEditor->viewport()->setFocusPolicy(Qt::StrongFocus);
    }
#endif
}

TextEditorTouchHandler::~TextEditorTouchHandler()
{
    if (m_textEditor) {
        disableGestureHandling(m_textEditor);
        QScroller::ungrabGesture(m_textEditor->viewport());
    }
}

void TextEditorTouchHandler::setupScrolling()
{
    if (!m_textEditor || !m_textEditor->viewport()) return;

#ifndef Q_OS_ANDROID
    m_scroller = QScroller::scroller(m_textEditor->viewport());
    QScrollerProperties props = m_scroller->scrollerProperties();
    
    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, 
                         QScrollerProperties::OvershootWhenScrollable);
    props.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.5);
    props.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.3);
    
    m_scroller->setScrollerProperties(props);
    
    // Only grab touch gestures on the viewport, not the entire editor
    // This prevents interference with toolbar buttons
    QScroller::grabGesture(m_textEditor->viewport(), QScroller::TouchGesture);
#endif
    
    updateScrollLimits();
}

void TextEditorTouchHandler::updateScrollLimits()
{
    if (!m_textEditor || !m_textEditor->viewport()) return;

    const int viewportHeight = m_textEditor->viewport()->height();
    const int contentHeight = m_textEditor->document()->size().height();
    
    const int min = 0;
    const int max = qMax(0, contentHeight - viewportHeight);
    
    // Set scroll limits for bounce effect
    m_touchInteraction->setScrollLimits(min, max);
}

void TextEditorTouchHandler::handlePinchGesture(QPinchGesture* gesture)
{
    if (!gesture || !m_textEditor || !m_touchInteraction) return;

    if (gesture->state() == Qt::GestureStarted) {
        m_currentScale = m_textEditor->zoomFactor();
    }

    if (gesture->changeFlags() & QPinchGesture::ScaleFactorChanged) {
        qreal newScale = m_currentScale * gesture->totalScaleFactor();
        newScale = qBound(MIN_SCALE, newScale, MAX_SCALE);
        
        if (!qFuzzyCompare(newScale, m_textEditor->zoomFactor())) {
            m_touchInteraction->setBounceScale(newScale);
        }
    }
}

void TextEditorTouchHandler::handleSwipeGesture(QSwipeGesture* gesture)
{
    if (!gesture || !m_textEditor) return;

    // Handle horizontal swipes for navigation
    if (gesture->state() == Qt::GestureFinished) {
        if (gesture->horizontalDirection() == QSwipeGesture::Left) {
            // Navigate forward in history
        } else if (gesture->horizontalDirection() == QSwipeGesture::Right) {
            // Navigate backward in history
        }
    }
}

void TextEditorTouchHandler::handlePanGesture(QPanGesture* gesture)
{
    // Pan gesture handling is managed by QScroller
}

bool TextEditorTouchHandler::handleTouchBegin(QTouchEvent* event)
{
    // Check if the touch is on the toolbar
    if (!event->points().isEmpty()) {
        QPoint touchPoint = event->points().first().position().toPoint();
        
        // If we have a text editor and a toolbar, check if touch is on toolbar
        if (m_textEditor) {
            QToolBar* toolbar = m_textEditor->findChild<QToolBar*>();
            if (toolbar && toolbar->geometry().contains(touchPoint)) {
                // Don't handle toolbar touches here, let them propagate to toolbar buttons
                return false;
            }
        }
    }
    
    bool handled = TouchInteractionHandler::handleTouchBegin(event);
    if (handled && m_scroller) {
        m_lastOverscrollAmount = 0.0;
    }
    return handled;
}

bool TextEditorTouchHandler::handleTouchUpdate(QTouchEvent* event)
{
    // Check if the touch is on the toolbar
    if (!event->points().isEmpty()) {
        QPoint touchPoint = event->points().first().position().toPoint();
        
        // If we have a text editor and a toolbar, check if touch is on toolbar
        if (m_textEditor) {
            QToolBar* toolbar = m_textEditor->findChild<QToolBar*>();
            if (toolbar && toolbar->geometry().contains(touchPoint)) {
                // Don't handle toolbar touches here, let them propagate to toolbar buttons
                return false;
            }
        }
    }
    
    bool handled = TouchInteractionHandler::handleTouchUpdate(event);
    if (handled && m_scroller && m_touchInteraction) {
        // Get the current scroll position to calculate overscroll
        QScrollBar *vScrollBar = m_textEditor->verticalScrollBar();
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
    return handled;
}

bool TextEditorTouchHandler::handleTouchEnd(QTouchEvent* event)
{
    // Check if the touch is on the toolbar
    if (!event->points().isEmpty()) {
        QPoint touchPoint = event->points().first().position().toPoint();
        
        // If we have a text editor and a toolbar, check if touch is on toolbar
        if (m_textEditor) {
            QToolBar* toolbar = m_textEditor->findChild<QToolBar*>();
            if (toolbar && toolbar->geometry().contains(touchPoint)) {
                // Don't handle toolbar touches here, let them propagate to toolbar buttons
                return false;
            }
        }
    }
    
    bool handled = TouchInteractionHandler::handleTouchEnd(event);
    if (handled && m_scroller) {
        emit overscrollAmountChanged(0.0);
    }
    return handled;
}