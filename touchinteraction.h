#ifndef TOUCHINTERACTION_H
#define TOUCHINTERACTION_H

#include <QObject>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QPointF>
#include <QTouchEvent>
#include <QGestureEvent>
#include <QPointer>
#include <QPinchGesture>
#include <QEasingCurve>
#include "physicsengine.h"

class TouchInteraction : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal bounceScale READ bounceScale WRITE setBounceScale NOTIFY bounceScaleChanged)
    Q_PROPERTY(qreal overscrollAmount READ overscrollAmount WRITE setOverscrollAmount NOTIFY overscrollAmountChanged)
    Q_PROPERTY(qreal jellyStrength READ jellyStrength WRITE setJellyStrength NOTIFY jellyStrengthChanged)
    Q_PROPERTY(qreal friction READ friction WRITE setFriction NOTIFY frictionChanged)

public:
    enum BouncePreset {
        Subtle,    // Light bounce with quick recovery
        Normal,    // Standard jelly-like bounce
        Playful,   // Exaggerated bounce with more oscillations
        Custom     // Use custom strength and friction values
    };
    Q_ENUM(BouncePreset)

    explicit TouchInteraction(QObject *parent = nullptr);

    // Bounce animation control
    qreal bounceScale() const { return m_bounceScale; }
    void setBounceScale(qreal scale);
    
    // Overscroll control
    qreal overscrollAmount() const { return m_overscrollAmount; }
    void setOverscrollAmount(qreal amount);
    
    // Jelly physics parameters
    qreal jellyStrength() const { return m_jellyStrength; }
    void setJellyStrength(qreal strength);
    
    qreal friction() const { return m_friction; }
    void setFriction(qreal friction);

    // Animation configuration
    void setBounceDuration(int ms) { m_bounceDuration = ms; }
    void setOverscrollDuration(int ms) { m_overscrollDuration = ms; }
    void setBounceCurve(const QEasingCurve &curve) { m_bounceCurve = curve; }
    void setBouncePreset(BouncePreset preset);
    
    // Scroll limits
    void setScrollLimits(qreal min, qreal max);
    bool isWithinLimits(qreal value) const;

    // Touch event handlers
    bool handleTouchEvent(QTouchEvent *event);
    bool handleGestureEvent(QGestureEvent *event);

    // Animation helpers
    void startBounceAnimation(qreal targetScale);

Q_SIGNALS:
    void bounceScaleChanged(qreal scale);
    void overscrollAmountChanged(qreal amount);
    void jellyStrengthChanged(qreal strength);
    void frictionChanged(qreal friction);

    // Touch lifecycle signals
    void touchBegin(const QPointF &pos);
    void touchMove(const QPointF &pos);
    void touchEnd();
    void pinchScaleChanged(qreal scale);
    void panDeltaChanged(const QPointF &delta);
    void scrollLimitReached(qreal position);

protected:
    // Animation helpers
    void startJellyOverscrollAnimation(qreal currentPos, qreal targetPos);
    void updateJellyPhysics(qreal deltaTime);
    QPointF calculateJellyForce(qreal displacement, qreal velocity) const;

private:
    void handlePinchGesture(QPinchGesture *gesture);
    void resetAnimations();
    void initializeJellyAnimation();

    // Animation and physics properties
    qreal m_bounceScale;
    qreal m_overscrollAmount;
    qreal m_jellyStrength;
    qreal m_friction;
    int m_bounceDuration;
    int m_overscrollDuration;
    QEasingCurve m_bounceCurve;

    // Physics engine
    PhysicsEngine *m_physicsEngine;
    bool m_isPhysicsActive;
    QPointer<QTimer> m_physicsTimer;

    // Animations
    QPointer<QPropertyAnimation> m_bounceAnimation;
    QPointer<QVariantAnimation> m_jellyAnimation;
    QPointer<QPropertyAnimation> m_overscrollAnimation;

    // Jelly physics state structure
    struct JellyState {
        qreal position = 0.0;
        qreal velocity = 0.0;
        qreal targetPosition = 0.0;
        bool active = false;
        double lastTime = 0.0;
    } m_jellyState;

    // Touch tracking
    QPointF m_lastTouchPoint;
    qreal m_currentPinchScale;
    qreal m_scrollMin;
    qreal m_scrollMax;
    bool m_isAnimating;
};

#endif // TOUCHINTERACTION_H