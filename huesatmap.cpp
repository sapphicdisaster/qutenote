#include "huesatmap.h"
#include <QPainter>
#include <QPen>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QTouchEvent>
#include <QMouseEvent>
#include <QPainterPath>
#include <QRadialGradient>
#include <Qt>
#include <QDebug>
#include "resourcemanager.h"

HueSatMap::HueSatMap(QWidget *parent)
    : QWidget(parent)
    , m_indicatorColor(Qt::white)
    , m_indicatorPos(-1, -1)
    , m_scale(1.0)
    , m_targetRadius(MIN_TOUCH_TARGET / 2.0)
    , m_isDragging(false)
{
    initializeComponent();
}

void HueSatMap::initializeComponent()
{
    // Initialize touch interaction
    m_touchInteraction = QuteNote::makeOwned<TouchInteraction>(this);
    m_gradientCache = HueSatMapCache::instance();

    // Set up widget attributes
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Setup connections
    setupConnections();
    
    // Initialize with a valid image
    resetGradient(256, 256);
}

void HueSatMap::setupConnections()
{
    // Connect touch interaction signals
    connect(m_touchInteraction.get(), &TouchInteraction::touchBegin,
            this, [this](const QPointF &pos) {
                m_isDragging = true;
                setIndicatorPos(pos);
                // Ensure hue is in valid range [0, 359] and saturation is in [0, 255]
                int hue = qBound(0, static_cast<int>(pos.x() / width() * 359), 359);
                int saturation = qBound(0, static_cast<int>((1.0 - pos.y() / height()) * 255), 255);
                emit colorSelected(QColor::fromHsv(hue, saturation, 255));
            });

    connect(m_touchInteraction.get(), &TouchInteraction::touchMove,
            this, [this](const QPointF &pos) {
                if (m_isDragging) {
                    setIndicatorPos(pos);
                    // Ensure hue is in valid range [0, 359] and saturation is in [0, 255]
                    int hue = qBound(0, static_cast<int>(pos.x() / width() * 359), 359);
                    int saturation = qBound(0, static_cast<int>((1.0 - pos.y() / height()) * 255), 255);
                    emit colorSelected(QColor::fromHsv(hue, saturation, 255));
                }
            });

    connect(m_touchInteraction.get(), &TouchInteraction::touchEnd,
            this, [this]() {
                m_isDragging = false;
            });

}

void HueSatMap::cleanupResources()
{
    // Clear cached gradients
    m_hueSatMap = QImage();
    if (m_gradientCache) {
        m_gradientCache->clear();
    }

    // Clean up touch interaction
    m_touchInteraction = nullptr;
    m_gradientCache = nullptr;
}

void HueSatMap::handleMemoryWarning()
{
    // Clear gradient cache to reduce memory usage
    if (m_gradientCache) {
        m_gradientCache->clear();
    }

    // Regenerate current gradient at a lower resolution if needed
    if (!m_hueSatMap.isNull()) {
        resetGradient(width() / 2, height() / 2);
    }
}

void HueSatMap::resetGradient(int width, int height)
{
    const int minDimension = MIN_TOUCH_TARGET * 2;
    const int maxDimension = 4096;
    
    width = qBound(minDimension, width, maxDimension);
    height = qBound(minDimension, height, maxDimension);
    
    // Get cached gradient
    if (m_gradientCache) {
        m_hueSatMap = m_gradientCache->getOrGenerateGradient(QSize(width, height));
    }
    
    update();
}

void HueSatMap::updateGradient()
{
    // Ensure we have a valid image
    if (m_hueSatMap.isNull()) {
        resetGradient(256, 256);
        if (m_hueSatMap.isNull()) {
            return;
        }
    }
    
    const int width = m_hueSatMap.width();
    const int height = m_hueSatMap.height();
    
    if (width <= 0 || height <= 0) {
        qWarning("Invalid image dimensions: %dx%d", width, height);
        return;
    }
    
    try {
        // Fill with a solid color first
        m_hueSatMap.fill(Qt::transparent);
        
        // Pre-calculate values outside the loop for better performance
        const qreal invWidth = 1.0 / qMax(1, width - 1);
        const qreal invHeight = 1.0 / qMax(1, height - 1);
        
        // Generate the gradient
        for (int y = 0; y < height; ++y) {
            QRgb *scanLine = reinterpret_cast<QRgb*>(m_hueSatMap.scanLine(y));
            if (!scanLine) {
                qWarning("Failed to get scanline %d", y);
                continue;
            }
            
            for (int x = 0; x < width; ++x) {
                // Calculate hue and saturation with proper bounds checking
                const qreal hue = qBound(0.0, static_cast<qreal>(x) * invWidth, 1.0);
                const qreal sat = qBound(0.0, 1.0 - (static_cast<qreal>(y) * invHeight), 1.0);
                
                // Convert to HSV color with proper range checking
                const int h = qBound(0, static_cast<int>(hue * 359.0 + 0.5), 359);
                const int s = qBound(0, static_cast<int>(sat * 255.0 + 0.5), 255);
                
                QColor color;
                color.setHsv(h, s, 255);
                scanLine[x] = color.rgba();
            }
        }
        
        update();
    } catch (const std::exception &e) {
        qWarning("Error in updateGradient: %s", e.what());
    } catch (...) {
        qWarning("Unknown error in updateGradient");
    }
}

void HueSatMap::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Regenerate gradient when size changes
    if (m_hueSatMap.isNull() || 
        m_hueSatMap.width() != width() || 
        m_hueSatMap.height() != height()) {
        resetGradient(width(), height());
    }
}

void HueSatMap::setHueSatMap(const QImage &image)
{
    if (m_hueSatMap.cacheKey() != image.cacheKey()) {
        m_hueSatMap = image;
        update();
    }
}

void HueSatMap::setIndicatorPos(const QPointF &pos)
{
    if (m_indicatorPos != pos) {
        m_indicatorPos = pos;
        update();
    }
}

void HueSatMap::setIndicatorColor(const QColor &color)
{
    if (m_indicatorColor != color) {
        m_indicatorColor = color;
        update();
    }
}

void HueSatMap::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (!painter.isActive()) {
        qWarning("HueSatMap: Failed to initialize painter");
        return;
    }

    try {
        QRect paintRect = rect();
        if (paintRect.isEmpty()) {
            return;
        }

        // Ensure we have a valid image with non-zero size
        if (m_hueSatMap.isNull() || m_hueSatMap.size().isEmpty()) {
            resetGradient(paintRect.width(), paintRect.height());
            if (m_hueSatMap.isNull()) {
                return;
            }
        }

        // Draw the image directly at the widget's size
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        
        // Scale the image to fit the widget while maintaining aspect ratio
        QImage scaled = m_hueSatMap.scaled(paintRect.size(), 
                                         Qt::KeepAspectRatio, 
                                         Qt::SmoothTransformation);
        
        // Center the image in the widget
        QPoint offset((paintRect.width() - scaled.width()) / 2, 
                     (paintRect.height() - scaled.height()) / 2);
        
        painter.drawImage(offset, scaled);
        
        // Calculate the actual drawn area
        QRectF drawnRect(offset, scaled.size());
        
        // Draw the indicator if position is valid
        if (m_indicatorPos.x() >= 0 && m_indicatorPos.y() >= 0 &&
            m_indicatorPos.x() <= 1.0 && m_indicatorPos.y() <= 1.0) {
            
            const qreal radius = 8.0;
            
            // Calculate position within the drawn image
            QPointF indicatorPos(
                drawnRect.left() + m_indicatorPos.x() * drawnRect.width(),
                drawnRect.top() + m_indicatorPos.y() * drawnRect.height()
            );

            // Draw outer white circle with black border
            painter.setPen(QPen(Qt::black, 1.5));
            painter.setBrush(Qt::white);
            painter.drawEllipse(indicatorPos, radius, radius);

            // Draw inner circle with current color
            painter.setPen(Qt::NoPen);
            painter.setBrush(m_indicatorColor);
            painter.drawEllipse(indicatorPos, radius - 2, radius - 2);
        }
    } catch (const std::exception &e) {
        qWarning("Error in HueSatMap::paintEvent: %s", e.what());
    }
}

void HueSatMap::setScale(qreal scale)
{
    if (qFuzzyCompare(m_scale, scale))
        return;
    
    m_scale = scale;
    emit scaleChanged(scale);
}

void HueSatMap::setTargetRadius(qreal radius)
{
    if (qFuzzyCompare(m_targetRadius, radius))
        return;
    
    m_targetRadius = radius;
    emit targetRadiusChanged(radius);
}

bool HueSatMap::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
        QTouchEvent *touchEvent = dynamic_cast<QTouchEvent*>(event);
        if (!touchEvent) return QWidget::event(event);
        const QList<QEventPoint> &points = touchEvent->points();

        // Handle single touch point
        if (points.count() == 1) {
            const QEventPoint &point = points.first();
            QPointF pos = point.position();

            // Convert touch event to mouse event
            QMouseEvent mouseEvent(
                event->type() == QEvent::TouchBegin ? QEvent::MouseButtonPress :
                event->type() == QEvent::TouchEnd ? QEvent::MouseButtonRelease :
                QEvent::MouseMove,
                pos.toPoint(),
                point.globalPosition().toPoint(),
                Qt::LeftButton,
                Qt::LeftButton,
                Qt::NoModifier
            );

            // Forward to mouse event handlers
            if (event->type() == QEvent::TouchBegin) {
                mousePressEvent(&mouseEvent);
            } else if (event->type() == QEvent::TouchUpdate) {
                mouseMoveEvent(&mouseEvent);
            } else if (event->type() == QEvent::TouchEnd) {
                mouseReleaseEvent(&mouseEvent);
            }

            return true;
        }
        break;
    }
    default:
        break;
    }

    return QWidget::event(event);
}
