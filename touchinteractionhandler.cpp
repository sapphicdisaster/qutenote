#include "touchinteractionhandler.h"
#include <QWidget>
#include <QTouchEvent>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>

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
        case QEvent::Gesture:
            return handleGesture(static_cast<QGestureEvent*>(event)->gesture(Qt::PinchGesture)) ||
                   handleGesture(static_cast<QGestureEvent*>(event)->gesture(Qt::SwipeGesture)) ||
                   handleGesture(static_cast<QGestureEvent*>(event)->gesture(Qt::PanGesture));
        
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
        QWidget* child = widget->childAt(m_lastTouchPoint.toPoint());
        if (child && (qobject_cast<QPushButton*>(child) || 
                      qobject_cast<QSlider*>(child) || 
                      qobject_cast<QSpinBox*>(child) ||
                      qobject_cast<QCheckBox*>(child))) {
            // Don't handle touches on interactive elements
            m_gestureInProgress = false;
            return false;
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