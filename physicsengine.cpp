#include "physicsengine.h"

PhysicsEngine::PhysicsEngine(QObject *parent)
    : QObject(parent)
    , m_minimumTimestep(DEFAULT_MIN_TIMESTEP)
    , m_maximumTimestep(DEFAULT_MAX_TIMESTEP)
    , m_accumulatedTime(0)
    , m_lastFrameTime(0)
{
    // Set up update timer with default 60 Hz rate
    m_updateTimer.setInterval(16);
    connect(&m_updateTimer, &QTimer::timeout, this, &PhysicsEngine::updatePhysics);
}

PhysicsEngine::~PhysicsEngine()
{
    stop();
}

void PhysicsEngine::setUpdateInterval(int msecs)
{
    m_updateTimer.setInterval(msecs);
}

void PhysicsEngine::setMinimumTimestep(qreal msecs)
{
    m_minimumTimestep = qBound(1.0/1000.0, msecs, m_maximumTimestep);
}

void PhysicsEngine::setMaximumTimestep(qreal msecs)
{
    m_maximumTimestep = qBound(m_minimumTimestep, msecs, 1.0/10.0);
}

void PhysicsEngine::start()
{
    if (!m_updateTimer.isActive()) {
        m_elapsedTimer.start();
        m_lastFrameTime = 0;
        m_accumulatedTime = 0;
        m_updateTimer.start();
    }
}

void PhysicsEngine::stop()
{
    m_updateTimer.stop();
}

void PhysicsEngine::reset()
{
    stop();
    m_state.reset();
    m_accumulatedTime = 0;
    m_lastFrameTime = 0;
}

void PhysicsEngine::updatePhysics()
{
    // Calculate elapsed time since last frame in seconds
    qreal currentTime = m_elapsedTimer.elapsed() / 1000.0;
    qreal deltaTime = m_lastFrameTime > 0 ? currentTime - m_lastFrameTime : 0;
    m_lastFrameTime = currentTime;
    
    // Clamp deltaTime to prevent spiral of death
    deltaTime = qBound(m_minimumTimestep, deltaTime, m_maximumTimestep);
    
    // Add to accumulated time
    m_accumulatedTime += deltaTime;
    
    // Fixed timestep update with interpolation for smooth rendering
    while (m_accumulatedTime >= m_minimumTimestep) {
        m_state.applySpringForce(m_minimumTimestep);
        m_accumulatedTime -= m_minimumTimestep;
        
        // Check if simulation has settled
        if (isSimulationComplete()) {
            stop();
            emit simulationComplete();
            return;
        }
    }
    
    // Notify about state update
    emit stateUpdated(m_state);
}

bool PhysicsEngine::isSimulationComplete() const
{
    // Check if the system has settled (very low velocity and close to target)
    return qAbs(m_state.velocity) < VELOCITY_THRESHOLD &&
           qAbs(m_state.position - m_state.targetPosition) < POSITION_THRESHOLD;
}