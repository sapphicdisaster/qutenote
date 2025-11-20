#include "mousecontrol.h"
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>

#define DEPRECATION_WARNING qWarning() << "Warning: Using deprecated MouseControl class. Please migrate to TouchInteractionHandler for better touch support."

MouseControl::MouseControl(QObject *parent)
    : QObject(parent)
    , m_isDragging(false)
    , m_targetWidget(nullptr)
    , m_activeArea()
    , m_startPos()
    , m_lastPos()
{
    DEPRECATION_WARNING;
}

void MouseControl::handleMousePress(QMouseEvent *event, const QRectF &activeArea, QWidget *targetWidget)
{
    if (targetWidget) {
        m_targetWidget = targetWidget;
    }
    
    if (event->button() == Qt::LeftButton) {
        m_activeArea = activeArea;
        m_startPos = event->pos();
        m_lastPos = m_startPos;
        m_isDragging = true;
        event->accept();
    } else {
        event->ignore();
    }
}

void MouseControl::handleMouseMove(QMouseEvent *event,
                                 const std::function<void(const QPointF &, const QPointF &)> &onDrag,
                                 const QRectF &constrainArea,
                                 QWidget *targetWidget)
{
    if (targetWidget) {
        m_targetWidget = targetWidget;
    }
    
    if (m_isDragging && event->buttons() & Qt::LeftButton) {
        QPointF currentPos = event->pos();
        
        // If we have an active area and the mouse is outside it, ignore the move
        if (!m_activeArea.isNull() && !m_activeArea.contains(currentPos)) {
            event->ignore();
            return;
        }
        
        // Calculate delta from last position
        QPointF delta = currentPos - m_lastPos;
        
        // If we have a constraint area, adjust the position
        if (!constrainArea.isNull()) {
            QPointF constrainedPos = currentPos;
            if (!constrainArea.contains(constrainedPos)) {
                constrainedPos.setX(qMax(constrainArea.left(), qMin(constrainedPos.x(), constrainArea.right())));
                constrainedPos.setY(qMax(constrainArea.top(), qMin(constrainedPos.y(), constrainArea.bottom())));
                delta = constrainedPos - m_lastPos;
                currentPos = constrainedPos;
            }
        }
        
        // Call the drag callback if provided
        if (onDrag) {
            onDrag(delta, currentPos);
        }
        
        // Forward to target widget if needed
        if (m_targetWidget) {
            QPoint localPos = toTargetCoordinates(event->pos(), m_targetWidget);
            QMouseEvent localEvent(event->type(), localPos, event->globalPosition().toPoint(),
                                 event->button(), event->buttons(), event->modifiers());
            QApplication::sendEvent(m_targetWidget, &localEvent);
        }
        
        m_lastPos = currentPos;
        event->accept();
    } else {
        event->ignore();
    }
}

void MouseControl::handleMouseRelease(QMouseEvent *event, QWidget *targetWidget)
{
    if (targetWidget) {
        m_targetWidget = targetWidget;
    }
    
    if (event->button() == Qt::LeftButton && m_isDragging) {
        // Forward to target widget if needed
        if (m_targetWidget) {
            QPoint localPos = toTargetCoordinates(event->pos(), m_targetWidget);
            QMouseEvent localEvent(event->type(), localPos, event->globalPosition().toPoint(),
                                 event->button(), event->buttons(), event->modifiers());
            QApplication::sendEvent(m_targetWidget, &localEvent);
        }
        
        m_isDragging = false;
        event->accept();
    } else {
        event->ignore();
    }
}
