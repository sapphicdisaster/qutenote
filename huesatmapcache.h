#ifndef HUESATMAPCACHE_H
#define HUESATMAPCACHE_H

#include <QCache>
#include <QImage>
#include <QSize>
#include <QString>
#include <memory>

class HueSatMapCache {
public:
    static HueSatMapCache *instance();
    
    // Get or generate gradient for the given size
    QImage getOrGenerateGradient(const QSize &size);
    
    // Configure cache settings
    void setMaxMemoryUsage(qint64 bytes);
    void optimizeCache();
    
    // Clear the cache
    void clear();
    
private:
    HueSatMapCache();
    ~HueSatMapCache() = default;
    
    // Helper methods
    QString cacheKey(const QSize &size) const;
    QImage generateGradient(const QSize &size);
    void cacheGradient(const QString &key, const QImage &gradient);
    
    static HueSatMapCache *s_instance;
    QCache<QString, QImage> m_cache;
    qint64 m_maxMemory;
    
    // Cache configuration
    static const qint64 DEFAULT_MAX_MEMORY = 50 * 1024 * 1024; // 50MB
    static const int MIN_DIMENSION = 48;  // Minimum touch target size
    static const int MAX_DIMENSION = 4096; // Maximum reasonable size
};

#endif // HUESATMAPCACHE_H