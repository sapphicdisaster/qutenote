#ifndef MOUSECONTROL_H
#define MOUSECONTROL_H

#include <QObject>
#include <QWidget>
#include <QPointF>
#include <QRectF>

class [[deprecated("Use TouchInteractionHandler instead. MouseControl will be removed in a future version.")]]
MouseControl : public QObject
{
    Q_OBJECT

public:
    [[deprecated("Use TouchInteractionHandler instead. MouseControl will be removed in a future version.")]]
    explicit MouseControl(QObject *parent = nullptr);
    
    // Call this from your widget's mousePressEvent
    [[deprecated("Use TouchInteractionHandler::handleTouchBegin instead.")]]
    void handleMousePress(QMouseEvent *event, const QRectF &activeArea = QRectF(), QWidget *targetWidget = nullptr);
    
    // Call this from your widget's mouseMoveEvent
    [[deprecated("Use TouchInteractionHandler::handleTouchUpdate instead.")]]
    void handleMouseMove(QMouseEvent *event, 
                        const std::function<void(const QPointF &delta, const QPointF &position)> &onDrag = nullptr,
                        const QRectF &constrainArea = QRectF(),
                        QWidget *targetWidget = nullptr);
    
    // Call this from your widget's mouseReleaseEvent
    [[deprecated("Use TouchInteractionHandler::handleTouchEnd instead.")]]
    void handleMouseRelease(QMouseEvent *event, QWidget *targetWidget = nullptr);
    
    // Check if currently dragging
    [[deprecated("Use TouchInteractionHandler::isGestureActive instead.")]]
    bool isDragging() const { return m_isDragging; }
    
    // Get the start position of the drag
    [[deprecated("Use TouchInteractionHandler::gestureStartPosition instead.")]]
    QPointF startPosition() const { return m_startPos; }
    
    // Get the last position
    [[deprecated("Use TouchInteractionHandler::lastTouchPoint instead.")]]
    QPointF lastPosition() const { return m_lastPos; }
    
    // Set a target widget for event forwarding (e.g., a slider)
    [[deprecated("Use TouchInteractionHandler::setTargetWidget instead.")]]
    void setTargetWidget(QWidget *widget) { m_targetWidget = widget; }
    
    // Check if the mouse is over the active area
    bool isInActiveArea(const QPointF &pos) const {
        return m_activeArea.isNull() || m_activeArea.contains(pos);
    }
    
    // Convert position to target widget's coordinates
    QPoint toTargetCoordinates(const QPoint &pos, QWidget *target) const {
        return target ? target->mapFromParent(pos) : pos;
    }

private:
    bool m_isDragging;
    QPointF m_startPos;
    QPointF m_lastPos;
    QRectF m_activeArea;
    QWidget *m_targetWidget = nullptr;

};

#endif // MOUSECONTROL_H
