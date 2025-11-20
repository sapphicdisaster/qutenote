#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QObject>
#include <QCache>
#include <QMap>
#include <QTimer>
#include "smartpointers.h"

namespace QuteNote {

class ResourceManager : public QObject, public Singleton<ResourceManager>
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ResourceManager)
    friend class Singleton<ResourceManager>;

public:
    // Resource tracking
    void trackResource(const QString& id, qint64 size);
    void untrackResource(const QString& id);
    void setResourceLimit(qint64 maxBytes);
    
    // Memory monitoring
    qint64 totalMemoryUsage() const { return m_totalMemoryUsage; }
    qint64 resourceLimit() const { return m_resourceLimit; }
    bool isNearLimit() const;

    // Resource cleanup policies
    void setCleanupThreshold(double threshold); // 0.0 to 1.0
    void setCleanupInterval(int msecs);
    
    // Manual cleanup
    void cleanupUnusedResources();

Q_SIGNALS:
    void memoryWarning(qint64 currentUsage, qint64 limit);
    void resourceLimitExceeded(qint64 excess);
    void resourceCleanupNeeded();

protected:
    ResourceManager();
    ~ResourceManager();

private:
    void monitorMemoryUsage();
    void scheduleCleanup();
    void checkMemoryThresholds();

    struct ResourceInfo {
        qint64 size;
        qint64 lastAccessed;
    };

    QMap<QString, ResourceInfo> m_resources;
    qint64 m_totalMemoryUsage;
    qint64 m_resourceLimit;
    double m_cleanupThreshold;
    
    OwnedPtr<QTimer> m_monitorTimer;
    static const qint64 DEFAULT_RESOURCE_LIMIT = 100 * 1024 * 1024; // 100MB
    static constexpr double DEFAULT_CLEANUP_THRESHOLD = 0.8; // 80%
    static const int MONITOR_INTERVAL = 5000; // 5 seconds
};

} // namespace QuteNote

#endif // RESOURCEMANAGER_H