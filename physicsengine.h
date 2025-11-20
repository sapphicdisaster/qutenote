#ifndef PHYSICSENGINE_H
#define PHYSICSENGINE_H

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QPointF>
#include <QtMath>

class PhysicsState {
public:
    qreal position = 0.0;
    qreal velocity = 0.0;
    qreal acceleration = 0.0;
    qreal targetPosition = 0.0;
    qreal minLimit = 0.0;
    qreal maxLimit = 0.0;
    
    // Spring constants
    qreal springConstant = 300.0;  // Strength of the spring force
    qreal damping = 20.0;          // Damping coefficient
    
    void reset() {
        position = velocity = acceleration = 0.0;
    }
    
    void applySpringForce(qreal dt) {
        // Calculate spring force (F = -kx)
        qreal displacement = position - targetPosition;
        qreal springForce = -springConstant * displacement;
        
        // Calculate damping force (F = -cv)
        qreal dampingForce = -damping * velocity;
        
        // Sum forces and apply F = ma
        qreal totalForce = springForce + dampingForce;
        acceleration = totalForce;  // Assuming unit mass (m=1)
        
        // Semi-implicit Euler integration
        velocity += acceleration * dt;
        position += velocity * dt;
        
        // Apply limits if set
        if (minLimit != maxLimit) {
            if (position < minLimit) {
                position = minLimit;
                velocity = 0;
            } else if (position > maxLimit) {
                position = maxLimit;
                velocity = 0;
            }
        }
    }
};

class PhysicsEngine : public QObject {
    Q_OBJECT
    
public:
    explicit PhysicsEngine(QObject *parent = nullptr);
    ~PhysicsEngine();
    
    // Physics configuration
    void setUpdateInterval(int msecs);
    void setMinimumTimestep(qreal msecs);
    void setMaximumTimestep(qreal msecs);
    
    // Physics state
    // Use the correct QTimer member to report activity
    bool isActive() const { return m_updateTimer.isActive(); }
    void start();
    void stop();
    void reset();
    
    // State access
    PhysicsState& state() { return m_state; }
    const PhysicsState& state() const { return m_state; }
    
signals:
    void stateUpdated(const PhysicsState &state);
    void simulationComplete();
    
private slots:
    void updatePhysics();
    
private:
    bool isSimulationComplete() const;
    
    QTimer m_updateTimer;
    QElapsedTimer m_elapsedTimer;
    PhysicsState m_state;
    
    qreal m_minimumTimestep;
    qreal m_maximumTimestep;
    qreal m_accumulatedTime;
    qreal m_lastFrameTime;
    
    static constexpr qreal DEFAULT_MIN_TIMESTEP = 1.0/240.0;  // 240 Hz max
    static constexpr qreal DEFAULT_MAX_TIMESTEP = 1.0/30.0;   // 30 Hz min
    static constexpr qreal VELOCITY_THRESHOLD = 0.01;
    static constexpr qreal POSITION_THRESHOLD = 0.01;
};

#endif // PHYSICSENGINE_H