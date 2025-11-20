#ifndef TOUCHINTERACTIONHANDLER_H
#define TOUCHINTERACTIONHANDLER_H

#include <QObject>
#include <QGesture>
#include <QPinchGesture>
#include <QSwipeGesture>
#include <QPanGesture>

class TouchInteractionHandler : public QObject {
    Q_OBJECT

public:
    explicit TouchInteractionHandler(QObject* parent = nullptr);
    virtual ~TouchInteractionHandler() = default;

    // Enable/disable gesture handling
    void enableGestureHandling(QWidget* widget);
    void disableGestureHandling(QWidget* widget);

protected:
    // Pure virtual gesture handlers
    virtual void handlePinchGesture(QPinchGesture* gesture) = 0;
    virtual void handleSwipeGesture(QSwipeGesture* gesture) = 0;
    virtual void handlePanGesture(QPanGesture* gesture) = 0;

    // Common utility methods for derived classes
    bool eventFilter(QObject* watched, QEvent* event) override;
    virtual bool handleTouchBegin(QTouchEvent* event);
    virtual bool handleTouchUpdate(QTouchEvent* event);
    virtual bool handleTouchEnd(QTouchEvent* event);

private:
    bool handleGesture(QGesture* gesture);
    void setupGestureFlags(QWidget* widget);

    // Gesture state tracking
    QPointF m_lastTouchPoint;
    bool m_gestureInProgress{false};
};

#endif // TOUCHINTERACTIONHANDLER_H