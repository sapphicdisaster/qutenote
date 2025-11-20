#include "touchinteraction.h"
#include <QWidget>
#include <QtMath>
#include <QDateTime>

TouchInteraction::TouchInteraction(QObject *parent)
    : QObject(parent)
    , m_bounceScale(1.0)
    , m_overscrollAmount(0.0)
    , m_jellyStrength(0.3)  // Default jelly strength
    , m_friction(0.95)      // Default friction
    , m_bounceDuration(250)
    , m_overscrollDuration(300)
    , m_currentPinchScale(1.0)
    , m_scrollMin(0.0)
    , m_scrollMax(0.0)
    , m_isAnimating(false)
    , m_physicsTimer(new QTimer(this))
{
    m_bounceCurve.setType(QEasingCurve::OutElastic);
    m_bounceCurve.setAmplitude(0.5);
    m_bounceCurve.setPeriod(0.75);
    
    m_physicsEngine = new PhysicsEngine(this);
    m_isPhysicsActive = false;
    
    // Connect physics timer to update method
    connect(m_physicsTimer.data(), &QTimer::timeout, this, [this]() {
        qreal currentTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
        qreal deltaTime = m_jellyState.lastTime > 0 ? currentTime - m_jellyState.lastTime : 0;
        m_jellyState.lastTime = currentTime;
        updateJellyPhysics(deltaTime);
    });
    
    // Configure physics engine
    m_physicsEngine->setUpdateInterval(16); // Default to 60Hz updates
    
    // Connect physics engine signals
    connect(m_physicsEngine, &PhysicsEngine::stateUpdated, this, [this](const PhysicsState &state) {
        setOverscrollAmount(state.position);
        if (state.position <= state.minLimit || state.position >= state.maxLimit) {
            emit scrollLimitReached(state.position);
        }
    });
    
    connect(m_physicsEngine, &PhysicsEngine::simulationComplete, this, [this]() {
        m_isPhysicsActive = false;
    });
}

void TouchInteraction::setBounceScale(qreal scale)
{
    if (qFuzzyCompare(m_bounceScale, scale))
        return;
    
    m_bounceScale = scale;
    emit bounceScaleChanged(scale);
}

void TouchInteraction::setOverscrollAmount(qreal amount)
{
    if (qFuzzyCompare(m_overscrollAmount, amount))
        return;
    
    m_overscrollAmount = amount;
    emit overscrollAmountChanged(amount);
}

void TouchInteraction::startBounceAnimation(qreal targetScale)
{
    if (!m_bounceAnimation) {
        m_bounceAnimation = new QPropertyAnimation(this, "bounceScale", this);
        m_bounceAnimation->setEasingCurve(m_bounceCurve);
    }

    m_bounceAnimation->stop();
    m_bounceAnimation->setDuration(m_bounceDuration);
    m_bounceAnimation->setStartValue(m_bounceScale);
    m_bounceAnimation->setEndValue(targetScale);
    m_bounceAnimation->start();
}

bool TouchInteraction::handleTouchEvent(QTouchEvent *event)
{
    switch (event->type()) {
        case QEvent::TouchBegin: {
            m_lastTouchPoint = event->points().first().position();
            emit touchBegin(m_lastTouchPoint);
            m_physicsEngine->stop();
            m_isPhysicsActive = false;
            resetAnimations();
            return true;
        }
        
        case QEvent::TouchUpdate: {
            if (event->points().isEmpty())
                return false;

            const QPointF newPos = event->points().first().position();
            const QPointF delta = newPos - m_lastTouchPoint;
            emit touchMove(newPos);
            m_lastTouchPoint = newPos;
            
            // Use physics engine for smooth overscroll
            if (!isWithinLimits(m_overscrollAmount + delta.y())) {
                if (!m_isPhysicsActive) {
                    m_physicsEngine->state().position = m_overscrollAmount;
                    m_physicsEngine->state().velocity = delta.y() * 60; // Convert to units/second
                    m_physicsEngine->state().springConstant = 300 * m_jellyStrength;
                    m_physicsEngine->state().damping = 20 * m_friction;
                    m_physicsEngine->state().minLimit = m_scrollMin;
                    m_physicsEngine->state().maxLimit = m_scrollMax;
                    m_physicsEngine->start();
                    m_isPhysicsActive = true;
                } else {
                    m_physicsEngine->state().velocity += delta.y() * 60;
                }
            }
            
            emit panDeltaChanged(delta);
            return true;
        }
        
        case QEvent::TouchEnd: {
            emit touchEnd();
            if (!isWithinLimits(m_overscrollAmount)) {
                startJellyOverscrollAnimation(m_overscrollAmount, 0);
            }
            return true;
        }
        
        default:
            return false;
    }
}

bool TouchInteraction::handleGestureEvent(QGestureEvent *event)
{
    if (QPinchGesture *pinch = static_cast<QPinchGesture *>(event->gesture(Qt::PinchGesture))) {
        handlePinchGesture(pinch);
        return true;
    }
    return false;
}

void TouchInteraction::handlePinchGesture(QPinchGesture *gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        qreal scale = gesture->totalScaleFactor();
        m_currentPinchScale = scale;
        emit pinchScaleChanged(scale);
    }
    
    if (gesture->state() == Qt::GestureFinished) {
        // Animate back to normal scale if needed
        if (m_currentPinchScale != 1.0) {
            startBounceAnimation(1.0);
        }
    }
}

void TouchInteraction::initializeJellyAnimation()
{
    m_jellyState.position = 0;
    m_jellyState.velocity = 0;
    m_jellyState.targetPosition = 0;
    m_jellyState.active = false;
    m_jellyState.lastTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
}

void TouchInteraction::startJellyOverscrollAnimation(qreal currentPos, qreal targetPos)
{
    if (m_jellyAnimation && m_jellyAnimation->state() == QAbstractAnimation::Running) {
        m_jellyAnimation->stop();
    }
    
    m_jellyState.position = currentPos;
    m_jellyState.targetPosition = targetPos;
    m_jellyState.velocity = 0;
    m_jellyState.active = true;
    m_jellyState.lastTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    
    if (!m_physicsTimer->isActive()) {
        m_physicsTimer->start();
    }
}

void TouchInteraction::updateJellyPhysics(qreal deltaTime)
{
    if (!m_jellyState.active) {
        m_physicsTimer->stop();
        return;
    }
    
    // Calculate spring force and damping
    const qreal displacement = m_jellyState.position - m_jellyState.targetPosition;
    const qreal springForce = -m_jellyStrength * displacement;
    const qreal dampingForce = -m_jellyState.velocity * (1.0 - m_friction);
    
    // Update velocity and position
    m_jellyState.velocity += (springForce + dampingForce) * deltaTime;
    m_jellyState.position += m_jellyState.velocity * deltaTime;
    
    // Check if we should stop the animation
    const qreal tolerance = 0.01;
    if (qAbs(displacement) < tolerance && qAbs(m_jellyState.velocity) < tolerance) {
        m_jellyState.active = false;
        m_jellyState.position = m_jellyState.targetPosition;
        m_jellyState.velocity = 0;
        m_physicsTimer->stop();
    }
    
    // Emit the new position
    setOverscrollAmount(m_jellyState.position);
}

void TouchInteraction::resetAnimations()
{
    if (m_bounceAnimation)
        m_bounceAnimation->stop();
    if (m_jellyAnimation)
        m_jellyAnimation->stop();
    if (m_physicsTimer->isActive())
        m_physicsTimer->stop();
    
    m_jellyState.active = false;
    m_isAnimating = false;
}

void TouchInteraction::setBouncePreset(BouncePreset preset)
{
    switch (preset) {
    case Subtle:
        setJellyStrength(0.1);
        setFriction(0.98);
        m_bounceCurve.setType(QEasingCurve::OutCubic);
        break;
    case Normal:
        setJellyStrength(0.3);
        setFriction(0.95);
        m_bounceCurve.setType(QEasingCurve::OutElastic);
        m_bounceCurve.setAmplitude(0.5);
        m_bounceCurve.setPeriod(0.75);
        break;
    case Playful:
        setJellyStrength(0.7);
        setFriction(0.92);
        m_bounceCurve.setType(QEasingCurve::OutElastic);
        m_bounceCurve.setAmplitude(1.0);
        m_bounceCurve.setPeriod(0.5);
        break;
    case Custom:
        // Keep current values
        break;
    }
}

void TouchInteraction::setScrollLimits(qreal min, qreal max)
{
    m_scrollMin = min;
    m_scrollMax = max;
}

bool TouchInteraction::isWithinLimits(qreal value) const
{
    // If min and max are the same, there are no limits
    if (qFuzzyCompare(m_scrollMin, m_scrollMax))
        return true;
    
    // Check if value is within the limits (inclusive)
    return value >= m_scrollMin && value <= m_scrollMax;
}

void TouchInteraction::setJellyStrength(qreal strength)
{
    if (qFuzzyCompare(m_jellyStrength, strength))
        return;
    
    m_jellyStrength = strength;
    emit jellyStrengthChanged(strength);
}

void TouchInteraction::setFriction(qreal friction)
{
    if (qFuzzyCompare(m_friction, friction))
        return;
    
    m_friction = friction;
    emit frictionChanged(friction);
}