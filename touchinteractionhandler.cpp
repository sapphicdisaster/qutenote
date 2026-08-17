#include "touchinteractionhandler.h"
#include <QWidget>
#include <QTouchEvent>
#include <QAbstractButton>
#include <QSlider>
#include <QComboBox>
#include <QAbstractSpinBox>
#include <QScroller>

TouchInteractionHandler::TouchInteractionHandler(QObject* parent)
    : QObject(parent)
{
}

void TouchInteractionHandler::enableGestureHandling(QWidget* widget)
{
    if (!widget) return;
    
    setupGestureFlags(widget);
    widget->installEventFilter(this);
}

void TouchInteractionHandler::disableGestureHandling(QWidget* widget)
{
    if (!widget) return;
    
    widget->removeEventFilter(this);
    widget->ungrabGesture(Qt::PinchGesture);
    widget->ungrabGesture(Qt::SwipeGesture);
    widget->ungrabGesture(Qt::PanGesture);
}

bool TouchInteractionHandler::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
        case QEvent::Gesture: {
            QGestureEvent* gestureEvent = static_cast<QGestureEvent*>(event);
            if (QGesture* pinch = gestureEvent->gesture(Qt::PinchGesture)) {
                if (handleGesture(pinch)) return true;
            }
            if (QGesture* swipe = gestureEvent->gesture(Qt::SwipeGesture)) {
                if (handleGesture(swipe)) return true;
            }
            if (QGesture* pan = gestureEvent->gesture(Qt::PanGesture)) {
                if (handleGesture(pan)) return true;
            }
            return false;
        }
        
        case QEvent::TouchBegin:
            return handleTouchBegin(static_cast<QTouchEvent*>(event));
        
        case QEvent::TouchUpdate:
            return handleTouchUpdate(static_cast<QTouchEvent*>(event));
        
        case QEvent::TouchEnd:
            return handleTouchEnd(static_cast<QTouchEvent*>(event));
        
        default:
            return QObject::eventFilter(watched, event);
    }
}

bool TouchInteractionHandler::handleGesture(QGesture* gesture)
{
    if (!gesture) return false;

    switch (gesture->gestureType()) {
        case Qt::PinchGesture:
            handlePinchGesture(static_cast<QPinchGesture*>(gesture));
            return true;
        
        case Qt::SwipeGesture:
            handleSwipeGesture(static_cast<QSwipeGesture*>(gesture));
            return true;
        
        case Qt::PanGesture:
            handlePanGesture(static_cast<QPanGesture*>(gesture));
            return true;
        
        default:
            return false;
    }
}

void TouchInteractionHandler::setupGestureFlags(QWidget* widget)
{
    widget->grabGesture(Qt::PinchGesture);
    widget->grabGesture(Qt::SwipeGesture);
    widget->grabGesture(Qt::PanGesture);
    widget->setAttribute(Qt::WA_AcceptTouchEvents);
}

bool TouchInteractionHandler::handleTouchBegin(QTouchEvent* event)
{
    if (event->points().isEmpty()) return false;
    
    m_lastTouchPoint = event->points().first().position();
    m_gestureInProgress = true;
    
    // Check if the touch is on a button or other interactive element
    QWidget* widget = qobject_cast<QWidget*>(parent());
    if (widget) {
        QPoint localPos = widget->mapFromGlobal(event->points().first().globalPosition().toPoint());
        QWidget* child = widget->childAt(localPos);
        while (child && child != widget) {
            if (qobject_cast<QAbstractButton*>(child) || 
                qobject_cast<QSlider*>(child) || 
                qobject_cast<QComboBox*>(child) ||
                qobject_cast<QAbstractSpinBox*>(child)) {
                // Don't handle touches on interactive elements
                m_gestureInProgress = false;
                return false;
            }
            child = child->parentWidget();
        }
    }
    
    return true;
}

bool TouchInteractionHandler::handleTouchUpdate(QTouchEvent* event)
{
    if (!m_gestureInProgress || event->points().isEmpty()) return false;
    
    m_lastTouchPoint = event->points().first().position();
    return true;
}

bool TouchInteractionHandler::handleTouchEnd(QTouchEvent* event)
{
    m_gestureInProgress = false;
    return true;
}

QScroller* TouchInteractionHandler::setupQScroller(QWidget* target,
                                                    qreal overshootResistance,
                                                    qreal overshootDistance)
{
    if (!target) return nullptr;

    QScroller* scroller = QScroller::scroller(target);
    QScrollerProperties props = scroller->scrollerProperties();

    props.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                          QScrollerProperties::OvershootWhenScrollable);
    props.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, overshootResistance);
    props.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, overshootDistance);

    scroller->setScrollerProperties(props);
    QScroller::grabGesture(target, QScroller::TouchGesture);

    return scroller;
}

void TouchInteractionHandler::cleanupQScroller(QWidget* target)
{
    if (target) {
        QScroller::ungrabGesture(target);
    }
}