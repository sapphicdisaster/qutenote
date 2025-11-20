#include "resourcemanager.h"
#include <QDebug>
#include <QDateTime>

namespace QuteNote {

ResourceManager::ResourceManager()
    : m_totalMemoryUsage(0)
    , m_resourceLimit(DEFAULT_RESOURCE_LIMIT)
    , m_cleanupThreshold(DEFAULT_CLEANUP_THRESHOLD)
    , m_monitorTimer(makeOwned<QTimer>(this))
{
    m_monitorTimer->setInterval(MONITOR_INTERVAL);
    connect(m_monitorTimer.get(), &QTimer::timeout, this, &ResourceManager::monitorMemoryUsage);
    m_monitorTimer->start();
}

ResourceManager::~ResourceManager()
{
    cleanupUnusedResources();
}

void ResourceManager::trackResource(const QString& id, qint64 size)
{
    ResourceInfo info;
    info.size = size;
    info.lastAccessed = QDateTime::currentMSecsSinceEpoch();
    
    auto it = m_resources.find(id);
    if (it != m_resources.end()) {
        m_totalMemoryUsage -= it->size;
    }
    
    m_resources[id] = info;
    m_totalMemoryUsage += size;
    
    checkMemoryThresholds();
}

void ResourceManager::untrackResource(const QString& id)
{
    auto it = m_resources.find(id);
    if (it != m_resources.end()) {
        m_totalMemoryUsage -= it->size;
        m_resources.erase(it);
    }
}

void ResourceManager::setResourceLimit(qint64 maxBytes)
{
    if (maxBytes > 0) {
        m_resourceLimit = maxBytes;
        checkMemoryThresholds();
    }
}

void ResourceManager::setCleanupThreshold(double threshold)
{
    if (threshold > 0.0 && threshold <= 1.0) {
        m_cleanupThreshold = threshold;
        checkMemoryThresholds();
    }
}

void ResourceManager::setCleanupInterval(int msecs)
{
    if (msecs > 0) {
        m_monitorTimer->setInterval(msecs);
    }
}

bool ResourceManager::isNearLimit() const
{
    return m_totalMemoryUsage >= (m_resourceLimit * m_cleanupThreshold);
}

void ResourceManager::monitorMemoryUsage()
{
    checkMemoryThresholds();
    
    if (isNearLimit()) {
        emit resourceCleanupNeeded();
        cleanupUnusedResources();
    }
}

void ResourceManager::checkMemoryThresholds()
{
    if (m_totalMemoryUsage > m_resourceLimit) {
        emit resourceLimitExceeded(m_totalMemoryUsage - m_resourceLimit);
        cleanupUnusedResources();
    }
    else if (isNearLimit()) {
        emit memoryWarning(m_totalMemoryUsage, m_resourceLimit);
    }
}

void ResourceManager::cleanupUnusedResources()
{
    if (m_resources.isEmpty()) {
        return;
    }

    // Sort by last accessed time
    QMultiMap<qint64, QString> accessMap;
    for (auto it = m_resources.constBegin(); it != m_resources.constEnd(); ++it) {
        accessMap.insert(it->lastAccessed, it.key());
    }

    // Remove oldest resources until we're under threshold
    qint64 targetUsage = m_resourceLimit * m_cleanupThreshold;
    auto it = accessMap.begin();
    while (m_totalMemoryUsage > targetUsage && it != accessMap.end()) {
        QString id = it.value();
        auto resourceIt = m_resources.find(id);
        if (resourceIt != m_resources.end()) {
            m_totalMemoryUsage -= resourceIt->size;
            m_resources.erase(resourceIt);
        }
        ++it;
    }
}

} // namespace QuteNote