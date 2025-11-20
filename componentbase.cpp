#include "componentbase.h"
#include <QEvent>

namespace QuteNote {

ComponentBase::ComponentBase(QWidget* parent)
    : QWidget(parent)
    , m_initialized(false)
    , m_memoryUsage(0)
{
}

ComponentBase::~ComponentBase()
{
    cleanupResources();
}

void ComponentBase::initializeComponent()
{
    if (m_initialized) {
        return;
    }
    
    // Derived classes should:
    // 1. Initialize member variables
    // 2. Create UI elements if needed
    // 3. Setup initial state
    // 4. Call setupConnections()
    // 5. Call markInitialized()
    
    emit componentInitialized();
}

void ComponentBase::setupConnections()
{
    // Derived classes should:
    // 1. Connect signals/slots
    // 2. Setup event filters if needed
    // 3. Register for notifications
}

void ComponentBase::setupComponent()
{
    // Default implementation - derived classes can override
    // to perform component-specific setup
}

void ComponentBase::cleanupComponent()
{
    // Default implementation - derived classes can override
    // to perform component-specific cleanup
}

void ComponentBase::refreshComponent()
{
    // Default implementation - derived classes can override
    // to refresh the component's state
}

void ComponentBase::cleanupResources()
{
    if (!m_initialized) {
        return;
    }
    
    emit componentCleanupStarted();
    
    // Unregister all resources
    if (m_memoryUsage > 0) {
        untrackMemoryUsage(m_memoryUsage);
    }
    
    // Derived classes should:
    // 1. Release any held resources
    // 2. Clear caches
    // 3. Disconnect signals/slots
    // 4. Remove event filters
    
    m_initialized = false;
    emit componentCleanupFinished();
}

void ComponentBase::registerResource(const QString& id, qint64 size)
{
    ResourceManager::instance()->trackResource(id, size);
    trackMemoryUsage(size);
}

void ComponentBase::unregisterResource(const QString& id)
{
    auto* rm = ResourceManager::instance();
    qint64 size = rm->totalMemoryUsage();
    rm->untrackResource(id);
    qint64 newSize = rm->totalMemoryUsage();
    untrackMemoryUsage(size - newSize);
}

void ComponentBase::trackMemoryUsage(qint64 bytes)
{
    if (bytes <= 0) {
        return;
    }
    
    m_memoryUsage += bytes;
    emit memoryUsageChanged(m_memoryUsage);
}

void ComponentBase::untrackMemoryUsage(qint64 bytes)
{
    if (bytes <= 0 || bytes > m_memoryUsage) {
        return;
    }
    
    m_memoryUsage -= bytes;
    emit memoryUsageChanged(m_memoryUsage);
}

void ComponentBase::setComponentName(const QString& name)
{
    m_componentName = name;
    setObjectName(name);
}

void ComponentBase::customEvent(QEvent* event)
{
    // Handle custom events like memory warnings
    // Derived classes should call base implementation
    QObject::customEvent(event);
}

void ComponentBase::handleMemoryWarning()
{
    // Default implementation - derived classes can override
    // to implement custom memory reduction strategies
}

void ComponentBase::handleResourceLimit()
{
    // Default implementation - derived classes can override
    // to handle resource limit exceeded situations
    cleanupResources();
}

} // namespace QuteNote