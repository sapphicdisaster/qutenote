#include "huesatmapcache.h"
#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QThread>
#include <algorithm>
#include <omp.h> // Added missing include for OpenMP
#include <stdexcept> // Added missing include for std::exception

// Static member definitions
const qint64 HueSatMapCache::DEFAULT_MAX_MEMORY;
const int HueSatMapCache::MIN_DIMENSION;
const int HueSatMapCache::MAX_DIMENSION;

HueSatMapCache* HueSatMapCache::s_instance = nullptr;

HueSatMapCache* HueSatMapCache::instance()
{
    if (!s_instance) {
        s_instance = new HueSatMapCache();
    }
    return s_instance;
}

HueSatMapCache::HueSatMapCache()
    : m_maxMemory(DEFAULT_MAX_MEMORY)
{
    m_cache.setMaxCost(m_maxMemory);
}

QString HueSatMapCache::cacheKey(const QSize &size) const
{
    return QString("%1x%2").arg(size.width()).arg(size.height());
}

void HueSatMapCache::setMaxMemoryUsage(qint64 bytes)
{
    m_maxMemory = bytes;
    m_cache.setMaxCost(m_maxMemory);
    optimizeCache();
}

void HueSatMapCache::optimizeCache()
{
    // Remove least recently used items if we're over the memory limit
    while (m_cache.totalCost() > m_maxMemory) {
        if (!m_cache.remove(m_cache.keys().first())) {
            break; // Failed to remove any more items
        }
    }
}

void HueSatMapCache::clear()
{
    m_cache.clear();
}

QImage HueSatMapCache::getOrGenerateGradient(const QSize &size)
{
    // Clamp dimensions to reasonable values
    QSize clampedSize(
        qBound(MIN_DIMENSION, size.width(), MAX_DIMENSION),
        qBound(MIN_DIMENSION, size.height(), MAX_DIMENSION)
    );
    
    // Check cache first
    QString key = cacheKey(clampedSize);
    QImage *cached = m_cache.object(key);
    if (cached) {
        return *cached;
    }
    
    // Generate new gradient
    QImage gradient = generateGradient(clampedSize);
    cacheGradient(key, gradient);
    return gradient;
}

void HueSatMapCache::cacheGradient(const QString &key, const QImage &gradient)
{
    // Calculate memory cost (width * height * bytes per pixel)
    int cost = gradient.width() * gradient.height() * 4; // RGBA = 4 bytes
    
    // Create a new cached image and insert into cache
    QImage *cached = new QImage(gradient);
    if (!m_cache.insert(key, cached, cost)) {
        delete cached; // Failed to insert, clean up
        qWarning() << "Failed to cache gradient with key:" << key;
    }
}

QImage HueSatMapCache::generateGradient(const QSize &size)
{
    QImage gradient(size, QImage::Format_ARGB32_Premultiplied);
    gradient.fill(Qt::transparent);
    
    const int width = size.width();
    const int height = size.height();
    const qreal invWidth = 1.0 / qMax(1, width - 1);
    const qreal invHeight = 1.0 / qMax(1, height - 1);
    
    try {
        // Generate gradient with optimized calculations
        #pragma omp parallel for collapse(2) if(width * height > 100000)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Calculate hue and saturation with proper bounds checking
                const qreal hue = qBound(0.0, static_cast<qreal>(x) * invWidth, 1.0);
                const qreal sat = qBound(0.0, 1.0 - (static_cast<qreal>(y) * invHeight), 1.0);
                
                // Convert to HSV color with proper range checking
                const int h = qBound(0, static_cast<int>(hue * 359.0 + 0.5), 359);
                const int s = qBound(0, static_cast<int>(sat * 255.0 + 0.5), 255);
                
                QColor color;
                color.setHsv(h, s, 255); // Use setHsv with integer values
                gradient.setPixel(x, y, color.rgba());
            }
        }
    } catch (const std::exception &e) {
        qWarning() << "Error generating gradient:" << e.what();
        gradient.fill(Qt::red); // Visual indicator of error
    }
    
    return gradient;
}